#version 450 core
#ifdef VERTEX_SHADER
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	float width;
	float height;
	float Near;
	float Far;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBinormal; 
layout(location = 4) in vec2 inTexCoord;
layout(location = 5) in vec4 a_MRow0;
layout(location = 6) in vec4 a_MRow1;
layout(location = 7) in vec4 a_MRow2;
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragWorldNormal;
layout(location = 2) out vec3 fragWorldTangent;
layout(location = 3) out vec3 fragWorldBinormal;
layout(location = 4) out vec2 fragTexCoord;

void main() {
    mat4 transform = mat4(
		vec4(a_MRow0.x, a_MRow1.x, a_MRow2.x, 0.0),
		vec4(a_MRow0.y, a_MRow1.y, a_MRow2.y, 0.0),
		vec4(a_MRow0.z, a_MRow1.z, a_MRow2.z, 0.0),
		vec4(a_MRow0.w, a_MRow1.w, a_MRow2.w, 1.0)
	);
    vec4 worldPos = transform * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    mat3 modelRot = mat3(transform);
    fragWorldNormal = normalize(modelRot * inNormal);
    fragWorldTangent = normalize(modelRot * inTangent);
    fragWorldBinormal = normalize(modelRot * inBinormal);
    fragTexCoord = inTexCoord;
    gl_Position = ubo.proj * ubo.view * worldPos;
}
#endif

#ifdef FRAGMENT_SHADER
layout(set = 1, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 1, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 1, binding = 2) uniform sampler2D u_MetalnessTexture;
layout(set = 1, binding = 3) uniform sampler2D u_RoughnessTexture;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec3 fragWorldTangent;
layout(location = 3) in vec3 fragWorldBinormal;
layout(location = 4) in vec2 fragTexCoord;

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gMR;

void main() {
    vec3 albedo = texture(u_AlbedoTexture, fragTexCoord).rgb;
    vec3 normalMap = texture(u_NormalTexture, fragTexCoord).rgb;
    float metalness = texture(u_MetalnessTexture, fragTexCoord).r;
    float roughness = texture(u_RoughnessTexture, fragTexCoord).g;

    mat3 TBN = mat3(
        normalize(fragWorldTangent),
        normalize(fragWorldBinormal),
        normalize(fragWorldNormal)
    );
    vec3 tangentNormal = normalMap * 2.0 - 1.0;
    vec3 worldNormal = normalize(TBN * tangentNormal);

    gPosition = vec4(fragWorldPos,1);
    gNormal = vec4(worldNormal,1); 
    gAlbedo = vec4(albedo,1);
    gMR = vec4(metalness, roughness,1,1);
}
#endif
