#ifndef vcSensor_h__
#define vcSensor_h__

#include <udMath.h>
#include <android/sensor.h>

const int LOOPER_ID_USER = 3;
const int SENSOR_HISTORY_LENGTH = 100;
const int SENSOR_REFRESH_RATE_HZ = 100;
constexpr int32_t SENSOR_REFRESH_PERIOD_US = int32_t(1000000 / SENSOR_REFRESH_RATE_HZ);
const float SENSOR_FILTER_ALPHA = 0.1f;

// define missing ASENSOR_TYPEs defined in Android source missing in NDK
// https://developer.android.com/ndk/reference/group/sensor
enum {
  ASENSOR_TYPE_GAME_ROTATION_VECTOR = 15
};

// Rotation Vector Sensor Handler
struct RVector {
  ASensorManager* pSensorManager;
  const ASensor* pRVectorSensor;
  ASensorEventQueue* pEventQueue;
  ALooper* pLooper;

  // All values are in radians/second and measure the rate of rotation around the X, Y and Z axis.
  udDouble3 data[SENSOR_HISTORY_LENGTH];
  udDouble3 dataFilter;
  udDouble3 value;
  int dataIndex;

  static RVector* init();
  static void update(RVector* pGyroSensor);
  //static udDouble3 get(RVector* pRVector);
};

#endif//vcSensor.h
