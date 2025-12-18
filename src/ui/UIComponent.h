#pragma once

namespace Image2Card::UI
{

class UIComponent
{
public:
    virtual ~UIComponent() = default;

    virtual void Render() = 0;
    virtual void Update() {}
};

} // namespace Image2Card::UI
