#include "cts/UnsafeArray.h"
#ifndef INTESTING

#include <stdio.h>

#define VERBOSE

#include <raylib.h>

#include "utils.h"
#include "cts/Class.h"
#include "classes/exploder.h"

int main() {
  START_LOGGING("game.log", LOG_INFO);
  LOG_BUILD_INFO();

  BeginClassRegistrations();
  RegisterClass(Exploder_ClassDef());
  EndClassRegistrations();

  MessagePayload msg = PreparePayload(CID_Exploder, MID_Exploder_ShimmiShimmiYea);
  Payload_SetValue(&msg, "Strength", float, 9.81f);
  DispatchMessage(&msg);
  FreePayload(&msg);

  MessagePayload msg2 = PreparePayload(CID_Exploder, MID_Exploder_ShimmiShimmiYea);
  Payload_SetValue(&msg2, "Strength", float, 0.03f);
  DispatchMessage(&msg2);
  FreePayload(&msg2);

  MessagePayload msg3 = PreparePayload(CID_Exploder, MID_Exploder_ShimmiShimmiYea);
  Payload_SetValue(&msg3, "Strength", float, 3.77f);
  DispatchMessage(&msg3);
  FreePayload(&msg3);

  return 0;
}

#endif