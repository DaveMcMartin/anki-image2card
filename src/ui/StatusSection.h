#pragma once

#include <string>

#include "ui/UIComponent.h"

namespace Image2Card::UI
{

  class StatusSection : public UIComponent
  {
public:

    StatusSection();
    ~StatusSection() override;

    void Render() override;

    void SetStatus(const std::string& status);
    void SetProgress(float progress);

private:

    std::string m_StatusMessage;
    float m_Progress = -1.0f;
  };

} // namespace Image2Card::UI
