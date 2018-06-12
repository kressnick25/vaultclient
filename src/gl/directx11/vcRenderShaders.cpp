#include "gl/vcRenderShaders.h"
#include "udPlatform/udPlatformUtil.h"

const char* const g_udFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  struct PS_OUTPUT {
    float4 Color0 : COLOR0;
    float Depth0 : DEPTH0;
  };

  sampler sampler0;
  Texture2D texture0;

  sampler sampler1;
  Texture2D texture1;

  PS_OUTPUT main(PS_INPUT input) : SV_Target
  {
    PS_OUTPUT output;

    output.Color0 = texture0.Sample(sampler0, input.uv).bgra;
    output.Depth0 = texture1.Sample(sampler1, input.uv).x;

    return output;
  }
)shader";

const char* const g_udVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.f, 1.f);
    output.uv  = input.uv;
    return output;
  }
)shader";


const char* const g_terrainTileFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float3 colour : COLOR0;
    float2 uv : TEXCOORD0;
  };

  uniform float u_opacity;
  uniform sampler2D u_texture;
  uniform float3 u_debugColour;

  float4 main(PS_INPUT input) : SV_Target
  {
    //float4 col = texture0.Sample(sampler0, input.uv);
    //return float4(col.xyz * u_debugColour.xyz, u_opacity);

    return float4(1.0, 0.0, 1.0, 1.0);
  }
)shader";

const char* const g_terrainTileVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float3 colour : COLOR0;
    float2 uv : TEXCOORD0;
  };

  uniform float4x4 u_worldViewProjection0;
  uniform float4x4 u_worldViewProjection1;
  uniform float4x4 u_worldViewProjection2;
  uniform float4x4 u_worldViewProjection3;

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    float4 finalClipPos = float4(0.0, 0.0, 0.0, 0.0);

    // corner id is stored in the x component of the position attribute
    // note: could have precision issues on some devices

    if (input.pos.x == 0.0)
      finalClipPos = float4(0.0, 0.0, 0.0, 1.0) * u_worldViewProjection0;
    else if (input.pos.x == 1.0)
      finalClipPos = float4(0.0, 0.0, 0.0, 1.0) * u_worldViewProjection1;
    else if (input.pos.x == 2.0)
      finalClipPos = float4(0.0, 0.0, 0.0, 1.0) * u_worldViewProjection2;
    else if (input.pos.x == 4.0)
      finalClipPos = float4(0.0, 0.0, 0.0, 1.0) * u_worldViewProjection3;

    output.uv = input.uv;
    output.pos = finalClipPos;
  }
)shader";

const char* const g_vcSkyboxFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  sampler sampler0;
  Texture2DArray u_texture;
  //uniform float4x4 u_inverseViewProjection;

  float4 main(PS_INPUT input) : SV_Target
  {
    float2 uv = float2(input.uv.x, 1.0 - input.uv.y);

    // work out 3D point
    float4 point3D = /*u_inverseViewProjection */ float4(uv * float2(2.0, 2.0) - float2(1.0, 1.0), 1.0, 1.0);
    point3D.xyz = normalize(point3D.xyz / point3D.w);
    return u_texture.Sample(sampler0, point3D.xyz);
  }
)shader";

const char* const g_ImGuiVertexShader = R"vert(
  cbuffer vertexBuffer : register(b0)
  {
    float4x4 ProjectionMatrix;
  };

  struct VS_INPUT
  {
    float2 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
  }
)vert";

const char* const g_ImGuiFragmentShader = R"frag(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
    return out_col;
  }
)frag";
