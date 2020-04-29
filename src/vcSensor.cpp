#include "vcSensor.h"
#include <cassert>

RVector* RVector::init() {
  RVector* pRVector = udAllocType(RVector, 1, udAF_Zero);
  pRVector->pSensorManager = ASensorManager_getInstance();
  assert(pRVector->pSensorManager != NULL);
  pRVector->pRVectorSensor = ASensorManager_getDefaultSensor(pRVector->pSensorManager, ASENSOR_TYPE_GAME_ROTATION_VECTOR);
  assert(pRVector->pRVectorSensor != NULL);
  pRVector->pLooper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
  assert(pRVector->pLooper != NULL);
  pRVector->pEventQueue = ASensorManager_createEventQueue(pRVector->pSensorManager, pRVector->pLooper,
    LOOPER_ID_USER, NULL, NULL);
  assert(pRVector->pEventQueue != NULL);
  auto status = ASensorEventQueue_enableSensor(pRVector->pEventQueue, pRVector->pRVectorSensor);
  assert(status >= 0);
  status = ASensorEventQueue_setEventRate(pRVector->pEventQueue, pRVector->pRVectorSensor, SENSOR_REFRESH_PERIOD_US);
  assert(status >= 0);

  return pRVector;
}

void RVector::update(RVector *pRVector){
  ALooper_pollAll(0, NULL, NULL, NULL);
  ASensorEvent event;
  while (ASensorEventQueue_getEvents(pRVector->pEventQueue, &event, 1) > 0) {
    pRVector->value.x = event.vector.roll;
    pRVector->value.y = -event.vector.pitch;
    pRVector->value.z = -event.vector.z;
    //pRVector->value.w = event.vector.azimuth;
  };
}




