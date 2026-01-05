#include "AIDictionaryClient.h"

#include <nlohmann/json.hpp>
#include <stdexcept>

#include "ai/ITextAIProvider.h"
#include "language/ILanguage.h"

namespace Image2Card::Language::Dictionary
{

  AIDictionaryClient::AIDictionaryClient(std::shared_ptr<Image2Card::AI::ITextAIProvider> aiProvider,
                                         const Image2Card::Language::ILanguage& language)
      : m_AIProvider(std::move(aiProvider))
      , m_Language(language)
  {}

  DictionaryEntry AIDictionaryClient::LookupWord(const std::string& word, const std::string& headword)
  {
    if (!m_AIProvider) {
      throw std::runtime_error("AI dictionary not initialized");
    }

    const std::string lookupWord = headword.empty() ? word : headword;

    try {
      nlohmann::json result = m_AIProvider->AnalyzeSentence(lookupWord, lookupWord, m_Language);

      DictionaryEntry entry;
      entry.headword = lookupWord;

      if (result.contains("definition") && result["definition"].is_string()) {
        entry.definition = result["definition"].get<std::string>();
      }

      if (result.contains("part_of_speech") && result["part_of_speech"].is_string()) {
        entry.partOfSpeech = result["part_of_speech"].get<std::string>();
      }

      if (entry.definition.empty()) {
        entry.definition = "No definition available";
      }

      return entry;
    } catch (const std::exception& e) {
      throw std::runtime_error(std::string("AI dictionary lookup failed: ") + e.what());
    }
  }

  bool AIDictionaryClient::IsAvailable() const
  {
    return m_AIProvider != nullptr;
  }

} // namespace Image2Card::Language::Dictionary