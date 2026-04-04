#include "cts/UnsafeArray.h"
#ifndef INTESTING

#include <stdio.h>

#define VERBOSE

#include <raylib.h>

#include "utils.h"
#include "cts/Class.h"
#include "classes/exploder.h"

int main() {
  START_LOGGING("game", LOG_INFO);
  LOG_BUILD_INFO();

  BeginClassRegistrations();
  RegisterClass(Exploder_ClassDef());
  EndClassRegistrations();

  MessagePayload msg = PreparePayload(CID_Exploder, MID_Exploder_ShimmiShimmiYea);
  Payload_SetValue(&msg, "Strength", float, 9.81f);
  DispatchMessage(&msg);
  FreePayload(&msg);

  return 0;
}

#endif