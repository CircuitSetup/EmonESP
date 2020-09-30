#include <StreamSpy.h>

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif

#ifndef DEBUG_PORT
#define DEBUG_PORT Serial
#endif

#ifndef EMONTX_PORT
#define EMONTX_PORT Serial1
#endif

#ifndef DEBUG_LOG_BUFFER
#define DEBUG_LOG_BUFFER 512
#endif

StreamSpy SerialDebug(DEBUG_PORT);
StreamSpy SerialEmonTx(EMONTX_PORT);

void debug_setup()
{
  DEBUG_PORT.begin(115200);
  SerialDebug.begin(DEBUG_LOG_BUFFER);

  //EMONTX_PORT.begin(115200);
  //SerialEmonTx.begin(DEBUG_LOG_BUFFER);
}
