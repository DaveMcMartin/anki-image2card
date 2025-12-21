#include "Application.h"

int main(int argc, char* argv[])
{
  Image2Card::Application app("Anki Image2Card", 1280, 720);
  app.Run();

  return 0;
}
