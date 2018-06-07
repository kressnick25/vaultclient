#include "vcRender.h"

#include "gl/vcRenderShaders.h"
#include "gl/vcFramebuffer.h"
#include "gl/vcShader.h"
#include "gl/vcGLState.h"

#include "vcTerrain.h"
#include "vcGIS.h"

const int qrIndices[6] = { 0, 1, 2, 0, 2, 3 };
const vcSimpleVertex qrSqVertices[4]{ { { -1.f, 1.f, 0.f },{ 0, 0 } },{ { -1.f, -1.f, 0.f },{ 0, 1 } },{ { 1.f, -1.f, 0.f },{ 1, 1 } },{ { 1.f, 1.f, 0.f },{ 1, 0 } } };

struct vcUDRenderContext
{
  vdkRenderContext *pRenderer;
  vdkRenderView *pRenderView;
  uint32_t *pColorBuffer;
  float *pDepthBuffer;
  vcTexture *pColourTex;
  vcTexture *pDepthTex;

  struct
  {
    vcShader *pProgram;
    vcShaderUniform *uniform_texture;
    vcShaderUniform *uniform_depth;
  } presentShader;
};

struct vcRenderContext
{
  vdkContext *pVaultContext;
  vcSettings *pSettings;
  vcCamera *pCamera;

  udUInt2 sceneResolution;

  vcFramebuffer *pFramebuffer;
  vcTexture *pTexture;
  vcTexture *pDepthTexture;

  vcUDRenderContext udRenderContext;

  udDouble4x4 viewMatrix;
  udDouble4x4 projectionMatrix;
  udDouble4x4 skyboxProjMatrix;
  udDouble4x4 viewProjectionMatrix;

  vcTerrain *pTerrain;

  struct
  {
    vcShader *pProgram;
    vcShaderUniform *uniform_texture;
    vcShaderUniform *uniform_inverseViewProjection;
  } skyboxShader;

  vcMesh *pSkyboxMesh;
  vcTexture *pSkyboxCubeMapTexture;
};

udResult vcRender_RecreateUDView(vcRenderContext *pRenderContext);
udResult vcRender_RenderAndUploadUDToTexture(vcRenderContext *pRenderContext, vcRenderData &renderData);

udResult vcRender_Init(vcRenderContext **ppRenderContext, vcSettings *pSettings, vcCamera *pCamera, const udUInt2 &sceneResolution)
{
  udResult result = udR_Success;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = udAllocType(vcRenderContext, 1, udAF_Zero);
  UD_ERROR_NULL(pRenderContext, udR_MemoryAllocationFailure);

  UD_ERROR_IF(!vcShader_CreateFromText(&pRenderContext->udRenderContext.presentShader.pProgram, g_udVertexShader, g_udFragmentShader, vcSimpleVertex::LayoutType, UDARRAYSIZE(vcSimpleVertex::LayoutType)), udR_InternalError);
  UD_ERROR_IF(!vcShader_CreateFromText(&pRenderContext->skyboxShader.pProgram, g_udVertexShader, g_vcSkyboxFragmentShader, vcSimpleVertex::LayoutType, UDARRAYSIZE(vcSimpleVertex::LayoutType)), udR_InternalError);

  vcTexture_LoadCubemap(&pRenderContext->pSkyboxCubeMapTexture, "CloudWater.jpg");

  vcShader_Bind(pRenderContext->skyboxShader.pProgram);
  vcShader_GetUniformIndex(&pRenderContext->skyboxShader.uniform_texture, pRenderContext->skyboxShader.pProgram, "u_texture");
  vcShader_GetUniformIndex(&pRenderContext->skyboxShader.uniform_inverseViewProjection, pRenderContext->skyboxShader.pProgram, "u_inverseViewProjection");

  vcShader_Bind(pRenderContext->udRenderContext.presentShader.pProgram);
  vcShader_GetUniformIndex(&pRenderContext->udRenderContext.presentShader.uniform_texture, pRenderContext->udRenderContext.presentShader.pProgram, "u_texture");
  vcShader_GetUniformIndex(&pRenderContext->udRenderContext.presentShader.uniform_depth, pRenderContext->udRenderContext.presentShader.pProgram, "u_depth");

  vcMesh_CreateSimple(&pRenderContext->pSkyboxMesh, qrSqVertices, 4, qrIndices, 6);

  vcShader_Bind(nullptr);

  *ppRenderContext = pRenderContext;

  pRenderContext->pSettings = pSettings;
  pRenderContext->pCamera = pCamera;
  result = vcRender_ResizeScene(pRenderContext, sceneResolution.x, sceneResolution.y);
  if (result != udR_Success)
    goto epilogue;

epilogue:
  return result;
}

udResult vcRender_Destroy(vcRenderContext **ppRenderContext)
{
  if (ppRenderContext == nullptr || *ppRenderContext == nullptr)
    return udR_Success;

  udResult result = udR_Success;
  vcRenderContext *pRenderContext = nullptr;

  UD_ERROR_NULL(ppRenderContext, udR_InvalidParameter_);

  pRenderContext = *ppRenderContext;
  *ppRenderContext = nullptr;

  if (vcTerrain_Destroy(&pRenderContext->pTerrain) != udR_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderContext_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);

epilogue:
  vcTexture_Destroy(&pRenderContext->udRenderContext.pColourTex);
  vcTexture_Destroy(&pRenderContext->udRenderContext.pDepthTex);
  vcTexture_Destroy(&pRenderContext->pTexture);
  vcTexture_Destroy(&pRenderContext->pDepthTexture);
  vcFramebuffer_Destroy(&pRenderContext->pFramebuffer);

  udFree(pRenderContext);
  return result;
}

udResult vcRender_SetVaultContext(vcRenderContext *pRenderContext, vdkContext *pVaultContext)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  pRenderContext->pVaultContext = pVaultContext;

  if (vdkRenderContext_Create(pVaultContext, &pRenderContext->udRenderContext.pRenderer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

udResult vcRender_ResizeScene(vcRenderContext *pRenderContext, const uint32_t width, const uint32_t height)
{
  udResult result = udR_Success;
  float fov = pRenderContext->pSettings->camera.fieldOfView;
  float aspect = width / (float)height;
  float zNear = pRenderContext->pSettings->camera.nearPlane;
  float zFar = pRenderContext->pSettings->camera.farPlane;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);
  UD_ERROR_IF(width == 0, udR_InvalidParameter_);
  UD_ERROR_IF(height == 0, udR_InvalidParameter_);

  pRenderContext->sceneResolution.x = width;
  pRenderContext->sceneResolution.y = height;
  pRenderContext->projectionMatrix = udDouble4x4::perspective(fov, aspect, zNear, zFar);
  pRenderContext->skyboxProjMatrix = udDouble4x4::perspective(fov, aspect, 0.5f, 10000.f);

  //Resize CPU Targets
  udFree(pRenderContext->udRenderContext.pColorBuffer);
  udFree(pRenderContext->udRenderContext.pDepthBuffer);

  pRenderContext->udRenderContext.pColorBuffer = udAllocType(uint32_t, pRenderContext->sceneResolution.x*pRenderContext->sceneResolution.y, udAF_Zero);
  pRenderContext->udRenderContext.pDepthBuffer = udAllocType(float, pRenderContext->sceneResolution.x*pRenderContext->sceneResolution.y, udAF_Zero);

  //Resize GPU Targets
  vcTexture_Destroy(&pRenderContext->udRenderContext.pColourTex);
  vcTexture_Destroy(&pRenderContext->udRenderContext.pDepthTex);
  vcTexture_Create(&pRenderContext->udRenderContext.pColourTex, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, pRenderContext->udRenderContext.pColorBuffer);
  vcTexture_Create(&pRenderContext->udRenderContext.pDepthTex, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y, pRenderContext->udRenderContext.pDepthBuffer, vcTextureFormat_D32F);

  vcTexture_Destroy(&pRenderContext->pTexture);
  vcTexture_Destroy(&pRenderContext->pDepthTexture);
  vcFramebuffer_Destroy(&pRenderContext->pFramebuffer);
  vcTexture_Create(&pRenderContext->pTexture, width, height, nullptr, vcTextureFormat_RGBA8);
  vcTexture_Create(&pRenderContext->pDepthTexture, width, height, nullptr, vcTextureFormat_D32F);
  vcFramebuffer_Create(&pRenderContext->pFramebuffer, pRenderContext->pTexture, pRenderContext->pDepthTexture);

  if (pRenderContext->pVaultContext)
    vcRender_RecreateUDView(pRenderContext);

epilogue:
  return result;
}

vcTexture* vcRender_RenderScene(vcRenderContext *pRenderContext, vcRenderData &renderData, vcFramebuffer *pDefaultFramebuffer)
{
  float fov = pRenderContext->pSettings->camera.fieldOfView;
  float aspect = pRenderContext->sceneResolution.x / (float)pRenderContext->sceneResolution.y;
  float zNear = pRenderContext->pSettings->camera.nearPlane;
  float zFar = pRenderContext->pSettings->camera.farPlane;

  pRenderContext->projectionMatrix = udDouble4x4::perspective(fov, aspect, zNear, zFar);
  pRenderContext->skyboxProjMatrix = udDouble4x4::perspective(fov, aspect, 0.5f, 10000.f);

  pRenderContext->viewMatrix = renderData.cameraMatrix;
  pRenderContext->viewMatrix.inverse();
  pRenderContext->viewProjectionMatrix = pRenderContext->projectionMatrix * pRenderContext->viewMatrix;

  vcGLState_SetDepthMode(true, true);

  vcRender_RenderAndUploadUDToTexture(pRenderContext, renderData);

  vcGLState_SetViewport(0, 0, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

  vcFramebuffer_Bind(pRenderContext->pFramebuffer);
  vcFramebuffer_Clear(pRenderContext->pFramebuffer, 0xFF000000);

  vcShader_Bind(pRenderContext->udRenderContext.presentShader.pProgram);

  vcShader_BindTexture(pRenderContext->udRenderContext.presentShader.pProgram, pRenderContext->udRenderContext.pColourTex, 0, pRenderContext->udRenderContext.presentShader.uniform_texture);
  vcShader_BindTexture(pRenderContext->udRenderContext.presentShader.pProgram, pRenderContext->udRenderContext.pDepthTex, 1, pRenderContext->udRenderContext.presentShader.uniform_depth);

  vcMesh_RenderTriangles(pRenderContext->pSkyboxMesh, 2);

  vcRenderSkybox(pRenderContext);

  if (renderData.srid != 0 && pRenderContext->pSettings->maptiles.mapEnabled)
  {
    udDouble3 localCamPos = renderData.cameraMatrix.axis.t.toVector3();

    // Corners [nw, ne, sw, se]
    udDouble3 localCorners[4];
    udInt2 slippyCorners[4];

    int currentZoom = 21;

    // Cardinal Limits
    localCorners[0] = localCamPos + udDouble3::create(-pRenderContext->pSettings->camera.farPlane, +pRenderContext->pSettings->camera.farPlane, 0);
    localCorners[1] = localCamPos + udDouble3::create(+pRenderContext->pSettings->camera.farPlane, +pRenderContext->pSettings->camera.farPlane, 0);
    localCorners[2] = localCamPos + udDouble3::create(-pRenderContext->pSettings->camera.farPlane, -pRenderContext->pSettings->camera.farPlane, 0);
    localCorners[3] = localCamPos + udDouble3::create(+pRenderContext->pSettings->camera.farPlane, -pRenderContext->pSettings->camera.farPlane, 0);

    for (int i = 0; i < 4; ++i)
      vcGIS_LocalToSlippy(renderData.srid, &slippyCorners[i], localCorners[i], currentZoom);

    while (currentZoom > 0 && (slippyCorners[0] != slippyCorners[1] || slippyCorners[1] != slippyCorners[2] || slippyCorners[2] != slippyCorners[3]))
    {
      --currentZoom;

      for (int i = 0; i < 4; ++i)
        slippyCorners[i] /= 2;
    }

    for (int i = 0; i < 4; ++i)
      vcGIS_SlippyToLocal(renderData.srid, &localCorners[i], slippyCorners[0] + udInt2::create(i & 1, i / 2), currentZoom);

    udDouble3 localViewPos = renderData.cameraMatrix.axis.t.toVector3();
    double localViewSize = (1.0 / (1 << 19)) + udAbs(renderData.cameraMatrix.axis.t.z - pRenderContext->pSettings->maptiles.mapHeight) / 70000.0;

    // for now just rebuild terrain every frame
    vcTerrain_BuildTerrain(pRenderContext->pTerrain, renderData.srid, localCorners, udInt3::create(slippyCorners[0], currentZoom), localViewPos, localViewSize);
    vcTerrain_Render(pRenderContext->pTerrain, pRenderContext->viewProjectionMatrix);
  }

  vcShader_Bind(nullptr);

  vcFramebuffer_Bind(pDefaultFramebuffer);

  return pRenderContext->pTexture;
}

udResult vcRender_RecreateUDView(vcRenderContext *pRenderContext)
{
  udResult result = udR_Success;

  UD_ERROR_NULL(pRenderContext, udR_InvalidParameter_);

  if (pRenderContext->udRenderContext.pRenderView && vdkRenderView_Destroy(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_Create(pRenderContext->pVaultContext, &pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pRenderer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_SetTargets(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, pRenderContext->udRenderContext.pColorBuffer, 0, pRenderContext->udRenderContext.pDepthBuffer) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vdkRVM_Projection, pRenderContext->projectionMatrix.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

udResult vcRender_RenderAndUploadUDToTexture(vcRenderContext *pRenderContext, vcRenderData &renderData)
{
  udResult result = udR_Success;
  vdkModel **ppModels = nullptr;
  int numVisibleModels = 0;

  if (vdkRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vdkRVM_Projection, pRenderContext->projectionMatrix.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (vdkRenderView_SetMatrix(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderView, vdkRVM_View, pRenderContext->viewMatrix.a) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (renderData.models.length > 0)
    ppModels = udAllocStack(vdkModel*, renderData.models.length, udAF_None);

  for (size_t i = 0; i < renderData.models.length; ++i)
  {
    if (renderData.models[i]->modelVisible)
    {
      ppModels[numVisibleModels] = renderData.models[i]->pVaultModel;
      ++numVisibleModels;
    }
  }

  vdkRenderPicking picking;
  picking.x = renderData.mouse.x;
  picking.y = renderData.mouse.y;

  if (vdkRenderContext_RenderAdv(pRenderContext->pVaultContext, pRenderContext->udRenderContext.pRenderer, pRenderContext->udRenderContext.pRenderView, ppModels, (int)numVisibleModels, &picking) != vE_Success)
    UD_ERROR_SET(udR_InternalError);

  if (picking.hit != 0)
  {
    // More to be done here
    renderData.worldMousePos = udDouble3::create(picking.pointCenter[0], picking.pointCenter[1], picking.pointCenter[2]);
    renderData.pickingSuccess = true;
  }

  vcTexture_UploadPixels(pRenderContext->udRenderContext.pColourTex, pRenderContext->udRenderContext.pColorBuffer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);
  vcTexture_UploadPixels(pRenderContext->udRenderContext.pDepthTex, pRenderContext->udRenderContext.pDepthBuffer, pRenderContext->sceneResolution.x, pRenderContext->sceneResolution.y);

epilogue:
  udFreeStack(pModels);
  return result;
}

void vcRenderSkybox(vcRenderContext *pRenderContext)
{
  udFloat4x4 viewMatrixF = udFloat4x4::create(pRenderContext->viewMatrix);
  udFloat4x4 projectionMatrixF = udFloat4x4::create(pRenderContext->skyboxProjMatrix);
  udFloat4x4 viewProjMatrixF = projectionMatrixF * viewMatrixF;
  viewProjMatrixF.axis.t = udFloat4::create(0, 0, 0, 1);
  viewProjMatrixF.inverse();

  vcShader_Bind(pRenderContext->skyboxShader.pProgram);

  vcShader_BindTexture(pRenderContext->skyboxShader.pProgram, pRenderContext->pSkyboxCubeMapTexture, 0, pRenderContext->skyboxShader.uniform_texture);
  vcShader_SetUniform(pRenderContext->skyboxShader.uniform_inverseViewProjection, viewProjMatrixF);

  // Draw the skybox only at the far plane, where there is no geometry.
  // Drawing skybox here (after 'opaque' geometry) saves a bit on fill rate.
  //glDepthRangef(1.0f, 1.0f);
  //glDepthFunc(GL_LEQUAL);

  //vcMesh_RenderTriangles(pRenderContext->pSkyboxMesh, 2);

  //glDepthFunc(GL_LESS);
  //glDepthRangef(0.0f, 1.0f);

  vcShader_Bind(nullptr);
}

udResult vcRender_CreateTerrain(vcRenderContext *pRenderContext, vcSettings *pSettings)
{
  if (pRenderContext == nullptr || pSettings == nullptr)
    return udR_InvalidParameter_;

  udResult result = udR_Success;

  UD_ERROR_NULL(&pRenderContext, udR_InvalidParameter_);

  if (vcTerrain_Init(&pRenderContext->pTerrain, pSettings) != udR_Success)
    UD_ERROR_SET(udR_InternalError);

epilogue:
  return result;
}

udResult vcRender_DestroyTerrain(vcRenderContext *pRenderContext)
{
  if (pRenderContext->pTerrain == nullptr)
    return udR_Success;
  else
    return vcTerrain_Destroy(&(pRenderContext->pTerrain));
}
