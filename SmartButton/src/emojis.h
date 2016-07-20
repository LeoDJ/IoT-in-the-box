#include <Arduino.h>

struct emojis { //has to be URL encoded due to message transfer method
  String battery = "%F0%9F%94%8B";
  String plug = "%F0%9F%94%8C";
  String bell = "%F0%9F%94%94";
};
