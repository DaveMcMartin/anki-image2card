#include "SentenceAnalyzer.h"

#include <stdexcept>

#include "core/Logger.h"
#include "language/ILanguage.h"
#include "language/dictionary/JMDictionary.h"
#include "language/furigana/MecabBasedFuriganaGenerator.h"
#include "language/morphology/MecabAnalyzer.h"
#include "language/pitch_accent/PitchAccentDatabase.h"
#include "language/services/ILanguageService.h"
#include "language/translation/ITranslator.h"

namespace Image2Card::Language::Analyzer
{

  SentenceAnalyzer::SentenceAnalyzer()
      : m_LanguageServices(nullptr)
      , m_MorphAnalyzer(nullptr)
      , m_FuriganaGen(nullptr)
      , m_DictClient(nullptr)
      , m_PitchAccent(nullptr)
      , m_PreferredTranslatorId("")
  {}

  void SentenceAnalyzer::SetLanguageServices(const std::vector<std::unique_ptr<Services::ILanguageService>>* services)
  {
    m_LanguageServices = services;
  }

  void SentenceAnalyzer::SetPreferredTranslator(const std::string& translatorId)
  {
    m_PreferredTranslatorId = translatorId;
    AF_INFO("SentenceAnalyzer: Preferred translator set to '{}'", translatorId);
  }

  bool SentenceAnalyzer::Initialize(const std::string& basePath)
  {
    try {
      // Initialize MeCab analyzer
      m_MorphAnalyzer = std::make_shared<Morphology::MecabAnalyzer>();
      AF_INFO("MeCab analyzer initialized");

      // Initialize furigana generator
      m_FuriganaGen = std::make_shared<Furigana::MecabBasedFuriganaGenerator>(m_MorphAnalyzer);
      AF_INFO("Furigana generator initialized");

      // Initialize dictionary client
      try {
        std::string dbPath = basePath + "assets/jmdict.db";
        m_DictClient = std::make_shared<Dictionary::JMDictionary>(dbPath);
        AF_INFO("Dictionary client initialized");
      } catch (const std::exception& e) {
        AF_WARN("Failed to initialize dictionary client: {}", e.what());
        m_DictClient = nullptr;
      }

      // Initialize pitch accent database
      try {
        std::string pitchDbPath = basePath + "assets/pitch_accent.db";
        m_PitchAccent = std::make_shared<PitchAccent::PitchAccentDatabase>(pitchDbPath);
        AF_INFO("Pitch accent database initialized");
      } catch (const std::exception& e) {
        AF_WARN("Failed to initialize pitch accent database: {}", e.what());
        m_PitchAccent = nullptr;
      }

      return true;

    } catch (const std::exception& e) {
      AF_ERROR("Failed to initialize SentenceAnalyzer: {}", e.what());
      return false;
    }
  }

  nlohmann::json SentenceAnalyzer::AnalyzeSentence(const std::string& sentence,
                                                   const std::string& targetWord,
                                                   const ILanguage* language)
  {
    (void) language; // Not currently used

    nlohmann::json result;

    if (sentence.empty()) {
      result["error"] = "Sentence cannot be empty";
      return result;
    }

    if (!IsReady()) {
      result["error"] = "Analyzer not initialized";
      return result;
    }

    try {
      // Determine the target word
      std::string focusWord = targetWord.empty() ? SelectTargetWord(sentence) : targetWord;

      if (focusWord.empty()) {
        AF_WARN("Could not determine target word for sentence: {}", sentence);
        focusWord = "詞"; // Fallback
      }

      // Generate furigana for the sentence
      std::string sentenceWithFurigana = sentence;
      if (m_FuriganaGen) {
        try {
          sentenceWithFurigana = m_FuriganaGen->Generate(sentence);
        } catch (const std::exception& e) {
          AF_WARN("Failed to generate furigana: {}", e.what());
        }
      }

      // Get the dictionary form and reading of the target word
      std::string dictionaryForm = GetDictionaryForm(focusWord);
      std::string reading = GetReading(focusWord);

      // Generate furigana for the target word (use dictionary form if available)
      std::string targetWordFurigana;
      std::string wordForFurigana = dictionaryForm.empty() ? focusWord : dictionaryForm;
      if (m_FuriganaGen && !reading.empty()) {
        try {
          targetWordFurigana = m_FuriganaGen->GenerateForWord(wordForFurigana);
        } catch (const std::exception& e) {
          AF_WARN("Failed to generate target word furigana: {}", e.what());
          targetWordFurigana = wordForFurigana;
        }
      } else {
        targetWordFurigana = wordForFurigana;
      }

      // Look up the definition
      std::string definition;
      if (m_DictClient) {
        try {
          auto dictEntry = m_DictClient->LookupWord(focusWord, dictionaryForm);
          definition = dictEntry.definition;
        } catch (const std::exception& e) {
          AF_WARN("Failed to lookup definition: {}", e.what());
        }
      }

      // Translate the sentence using language services
      std::string translation;
      auto translator = GetTranslator();
      if (translator) {
        try {
          translation = translator->Translate(sentence);
        } catch (const std::exception& e) {
          AF_WARN("Translation failed: {}", e.what());
        }
      }

      // Look up pitch accent
      std::string pitchAccent;
      if (m_PitchAccent) {
        try {
          std::string lookupWord = dictionaryForm.empty() ? focusWord : dictionaryForm;
          auto pitchEntries = m_PitchAccent->LookupWord(lookupWord, reading);
          if (pitchEntries.empty() && !reading.empty()) {
            pitchEntries = m_PitchAccent->LookupWord(reading, reading);
          }
          pitchAccent = m_PitchAccent->FormatAsHtml(pitchEntries);
        } catch (const std::exception& e) {
          AF_WARN("Failed to lookup pitch accent: {}", e.what());
        }
      }

      // Highlight target word in sentence
      std::string highlightedSentence = sentence;
      size_t pos = highlightedSentence.find(focusWord);
      if (pos != std::string::npos) {
        highlightedSentence.replace(pos, focusWord.length(), "<b style=\"color: green;\">" + focusWord + "</b>");
      }

      // Highlight target word in furigana
      // Strategy: Replace the focusWord in the plain sentence with a marker,
      // then use the same marker position logic in the furigana string
      std::string highlightedFurigana = sentenceWithFurigana;

      size_t furiganaPos = highlightedFurigana.find(focusWord);
      if (furiganaPos != std::string::npos) {
        // Simple case: the exact word appears in furigana (all hiragana or not split by furigana brackets)
        highlightedFurigana.replace(
            furiganaPos, focusWord.length(), "<b style=\"color: green;\">" + focusWord + "</b>");
      } else {
        // Complex case: word might be split by furigana brackets like "素[そ]な"
        // We need to find where focusWord appears in the original sentence and highlight the same region in furigana
        size_t sentencePos = sentence.find(focusWord);
        if (sentencePos != std::string::npos) {
          // Build a version of furigana without brackets to find positions
          std::string furiganaWithoutBrackets;
          std::vector<size_t> positionMap; // Maps position in furiganaWithoutBrackets to position in original furigana

          for (size_t i = 0; i < highlightedFurigana.length(); ++i) {
            if (highlightedFurigana[i] == '[') {
              // Skip until ]
              while (i < highlightedFurigana.length() && highlightedFurigana[i] != ']') {
                i++;
              }
            } else if (highlightedFurigana[i] != ' ') {
              positionMap.push_back(i);
              furiganaWithoutBrackets += highlightedFurigana[i];
            }
          }

          // Find focusWord in the bracket-free version
          size_t cleanPos = furiganaWithoutBrackets.find(focusWord);
          if (cleanPos != std::string::npos && cleanPos < positionMap.size()) {
            size_t startPos = positionMap[cleanPos];
            size_t endIndex = cleanPos + focusWord.length() - 1;
            size_t endPos = (endIndex < positionMap.size()) ? positionMap[endIndex] + 1 : highlightedFurigana.length();

            // Check if endPos lands inside a furigana bracket and extend to include the closing ]
            if (endPos < highlightedFurigana.length()) {
              size_t scanPos = endPos;
              while (scanPos < highlightedFurigana.length() && highlightedFurigana[scanPos] != ' ') {
                if (highlightedFurigana[scanPos] == ']') {
                  endPos = scanPos + 1;
                  break;
                }
                scanPos++;
              }
            }

            std::string wordWithFurigana = highlightedFurigana.substr(startPos, endPos - startPos);
            highlightedFurigana.replace(
                startPos, endPos - startPos, "<b style=\"color: green;\">" + wordWithFurigana + "</b>");
          }
        }
      }

      // Build the result JSON
      result["sentence"] = highlightedSentence;
      result["translation"] = translation;
      result["target_word"] = dictionaryForm.empty() ? focusWord : dictionaryForm;
      result["target_word_furigana"] = targetWordFurigana;
      result["furigana"] = highlightedFurigana;
      result["definition"] = definition;
      result["pitch_accent"] = pitchAccent;

      AF_DEBUG("Analysis complete for sentence: {}", sentence);

      return result;

    } catch (const std::exception& e) {
      AF_ERROR("Failed to analyze sentence: {}", e.what());
      result["error"] = std::string("Analysis failed: ") + e.what();
      return result;
    }
  }

  bool SentenceAnalyzer::IsReady() const
  {
    return m_MorphAnalyzer && m_FuriganaGen;
  }

  std::shared_ptr<Translation::ITranslator> SentenceAnalyzer::GetTranslator() const
  {
    if (!m_LanguageServices) {
      AF_WARN("GetTranslator: No language services available");
      return nullptr;
    }

    AF_DEBUG("GetTranslator: Preferred translator ID = '{}'", m_PreferredTranslatorId);

    if (!m_PreferredTranslatorId.empty()) {
      for (const auto& service : *m_LanguageServices) {
        if (service->GetType() == "translator") {
          AF_DEBUG("GetTranslator: Checking translator '{}' (id: {}, available: {})",
                   service->GetName(),
                   service->GetId(),
                   service->IsAvailable());
          if (service->GetId() == m_PreferredTranslatorId && service->IsAvailable()) {
            AF_INFO("GetTranslator: Using preferred '{}' translator", service->GetId());
            return service->GetTranslator();
          }
        }
      }
      AF_WARN("GetTranslator: Preferred translator '{}' not found or not available, falling back to first available",
              m_PreferredTranslatorId);
    }

    for (const auto& service : *m_LanguageServices) {
      if (service->GetType() == "translator") {
        AF_DEBUG("GetTranslator: Fallback check - translator '{}' (id: {}, available: {})",
                 service->GetName(),
                 service->GetId(),
                 service->IsAvailable());
        if (service->IsAvailable()) {
          AF_INFO("GetTranslator: Using first available '{}' translator", service->GetId());
          return service->GetTranslator();
        }
      }
    }

    AF_ERROR("GetTranslator: No available translators found");
    return nullptr;
  }

  std::string SentenceAnalyzer::SelectTargetWord(const std::string& sentence)
  {
    if (!m_MorphAnalyzer) {
      return "";
    }

    try {
      auto tokens = m_MorphAnalyzer->Analyze(sentence);

      // Find the first content word (noun, verb, or adjective)
      for (const auto& token : tokens) {
        if (!token.surface.empty() &&
            (token.partOfSpeech == "名詞" || token.partOfSpeech == "動詞" || token.partOfSpeech == "形容詞"))
        {
          return token.surface;
        }
      }

      // If no content word found, return the first non-empty token
      for (const auto& token : tokens) {
        if (!token.surface.empty()) {
          return token.surface;
        }
      }
    } catch (const std::exception& e) {
      AF_WARN("Failed to select target word: {}", e.what());
    }

    return "";
  }

  std::string SentenceAnalyzer::GetDictionaryForm(const std::string& surface)
  {
    if (!m_MorphAnalyzer) {
      return surface;
    }

    try {
      return m_MorphAnalyzer->GetDictionaryForm(surface);
    } catch (const std::exception& e) {
      AF_WARN("Failed to get dictionary form: {}", e.what());
      return surface;
    }
  }

  std::string SentenceAnalyzer::GetReading(const std::string& surface)
  {
    if (!m_MorphAnalyzer) {
      return "";
    }

    try {
      return m_MorphAnalyzer->GetReading(surface);
    } catch (const std::exception& e) {
      AF_WARN("Failed to get reading: {}", e.what());
      return "";
    }
  }

} // namespace Image2Card::Language::Analyzer