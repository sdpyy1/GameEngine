#include "hzpch.h"
#include "Animation.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Asset/Model/Mesh.h"

#include <acl/core/ansi_allocator.h>
#include <acl/compression/compress.h>
#include <acl/decompression/decompress.h>

#include <glm/gtc/type_ptr.hpp>

namespace Hazel {

	namespace Utils {

		acl::iallocator& GetAnimationAllocator()
		{
			static acl::ansi_allocator s_Allocator;
			return s_Allocator;
		}


		struct RootBoneTrackWriter : public acl::track_writer
		{
			RootBoneTrackWriter(glm::vec3& translation, glm::quat& rotation)
				: m_Rotation(rotation)
				, m_Translation(translation)
			{
			}

			static constexpr bool skip_all_scales() { return true; }

			constexpr bool skip_track_rotation(uint32_t track_index) const { return track_index > 0; }
			constexpr bool skip_track_translation(uint32_t track_index) const { return track_index > 0; }
			constexpr bool skip_track_scale(uint32_t track_index) const { return true; }

			void RTM_SIMD_CALL write_rotation(uint32_t /*track_index*/, rtm::quatf_arg0 rotation)
			{
				rtm::quat_store(rotation, glm::value_ptr(m_Rotation));
			}

			void RTM_SIMD_CALL write_translation(uint32_t /*track_index*/, rtm::vector4f_arg0 translation)
			{
				rtm::vector_store3(translation, glm::value_ptr(m_Translation));
			}

		private:
			glm::quat& m_Rotation;
			glm::vec3& m_Translation;
		};

	}


	Animation::Animation(const float duration, const uint32_t numTracks, void* data)
		: m_Data(data)
		, m_Duration(duration)
		, m_NumTracks(numTracks)
		, m_RootTranslationStart(0.0f, 0.0f, 0.0f)
		, m_RootTranslationEnd(0.0f, 0.0f, 0.0f)
		, m_RootRotationStart(1.0f, 0.0f, 0.0f, 0.0f)
		, m_RootRotationEnd(1.0f, 0.0f, 0.0f, 0.0f)
	{
		Utils::RootBoneTrackWriter writeStart(m_RootTranslationStart, m_RootRotationStart);
		Utils::RootBoneTrackWriter writeEnd(m_RootTranslationEnd, m_RootRotationEnd);
		acl::decompression_context<acl::default_transform_decompression_settings> context;
		context.initialize(*static_cast<acl::compressed_tracks*>(data));
		context.seek(0, acl::sample_rounding_policy::none);
		context.decompress_track(0, writeStart);
		context.seek(m_Duration, acl::sample_rounding_policy::none);
		context.decompress_track(0, writeEnd);
	}


	Animation::Animation(Animation&& other)
		: m_Data(other.m_Data)
		, m_Duration(other.m_Duration)
		, m_NumTracks(other.m_NumTracks)
		, m_RootTranslationStart(other.m_RootTranslationStart)
		, m_RootTranslationEnd(other.m_RootTranslationEnd)
		, m_RootRotationStart(other.m_RootRotationStart)
		, m_RootRotationEnd(other.m_RootRotationEnd)
	{
		other.m_Data = nullptr; // we've moved the data, so make sure it doesn't get deleted
	}


	Animation& Animation::operator=(Animation&& other)
	{
		if (this != &other)
		{
			m_Data = other.m_Data;
			m_Duration = other.m_Duration;
			m_NumTracks = other.m_NumTracks;
			m_RootTranslationStart = other.m_RootTranslationStart;
			m_RootTranslationEnd = other.m_RootTranslationEnd;
			m_RootRotationStart = other.m_RootRotationStart;
			m_RootRotationEnd = other.m_RootRotationEnd;
			other.m_Data = nullptr; // we've moved the data, so make sure it doesn't get deleted
		}
		return *this;
	}


	Animation::~Animation()
	{
		if (m_Data)
		{
			acl::iallocator& allocator = Utils::GetAnimationAllocator();
			allocator.deallocate(m_Data, static_cast<acl::compressed_tracks*>(m_Data)->get_size());
		}
	}


	AnimationAsset::AnimationAsset(AssetHandle animationSource, AssetHandle skeletonSource, const std::string_view animationName, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, const float rootRotationMask)
		: m_RootTranslationMask(rootTranslationMask)
		, m_RootRotationMask(rootRotationMask)
		, m_AnimationSource(animationSource)
		, m_SkeletonSource(skeletonSource)
		, m_AnimationName(animationName)
		, m_IsMaskedRootMotion(isMaskedRootMotion)
	{
		HZ_CORE_ASSERT(rootRotationMask == 0.0f || rootRotationMask == 1.0f);
		HZ_CORE_ASSERT(rootTranslationMask.x == 0.0f || rootTranslationMask.x == 1.0f);
		HZ_CORE_ASSERT(rootTranslationMask.y == 0.0f || rootTranslationMask.y == 1.0f);
		HZ_CORE_ASSERT(rootTranslationMask.z == 0.0f || rootTranslationMask.z == 1.0f);
		AssetManager::RegisterDependency(skeletonSource, Handle);
		AssetManager::RegisterDependency(animationSource, Handle);
	}


	void Animation::Sample(float time, Pose& outPose) const
	{
		if (!m_Data)
			return;

		// =============== 1. 初始化 ACL 解压上下文 ===============
		acl::decompression_context<acl::default_transform_decompression_settings> context;
		const acl::compressed_tracks* tracks = static_cast<const acl::compressed_tracks*>(m_Data);
		context.initialize(*tracks);

		// =============== 2. 时间规整（循环播放） ===============
		if (m_Duration > 0.0f)
			time = fmod(time, m_Duration);
		else
			time = 0.0f;

		context.seek(time, acl::sample_rounding_policy::none);

		// =============== 3. 输出 Pose 初始化 ===============
		outPose.AnimationDuration = m_Duration;
		outPose.AnimationTimePos = time;
		outPose.NumBones = m_NumTracks;

		// 根运动（root motion）单独计算
		glm::vec3 rootTranslation(0.0f);
		glm::quat rootRotation(1.0f, 0.0f, 0.0f, 0.0f);

		// 我们用自定义 writer 写入骨骼变换
		struct TrackWriter : public acl::track_writer
		{
			std::array<LocalTransform, Animation::MAXBONES>& BoneTransforms;
			glm::vec3& RootTranslation;
			glm::quat& RootRotation;

			TrackWriter(std::array<LocalTransform, Animation::MAXBONES>& bones,
				glm::vec3& rootT, glm::quat& rootR)
				: BoneTransforms(bones), RootTranslation(rootT), RootRotation(rootR) {
			}

			void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation)
			{
				if (track_index >= Animation::MAXBONES) return;
				glm::quat q;
				rtm::quat_store(rotation, glm::value_ptr(q));
				if (track_index == 0)
					RootRotation = q;
				BoneTransforms[track_index].Rotation = q;
			}

			void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation)
			{
				if (track_index >= Animation::MAXBONES) return;
				glm::vec3 t;
				rtm::vector_store3(translation, glm::value_ptr(t));
				if (track_index == 0)
					RootTranslation = t;
				BoneTransforms[track_index].Translation = t;
			}

			void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale)
			{
				if (track_index >= Animation::MAXBONES) return;
				glm::vec3 s;
				rtm::vector_store3(scale, glm::value_ptr(s));
				BoneTransforms[track_index].Scale = s;
			}
		};

		// =============== 4. 解压当前帧的所有骨骼变换 ===============
		TrackWriter writer(outPose.BoneTransforms, rootTranslation, rootRotation);
		context.decompress_tracks(writer);

		// =============== 5. 记录根运动（Root Motion） ===============
		outPose.RootMotion.Translation = rootTranslation;
		outPose.RootMotion.Rotation = rootRotation;
		outPose.RootMotion.Scale = glm::one<glm::vec3>();
	}


	AssetHandle AnimationAsset::GetAnimationSource() const
	{
		return m_AnimationSource;
	}


	AssetHandle AnimationAsset::GetSkeletonSource() const
	{
		return m_SkeletonSource;
	}


	const std::string& AnimationAsset::GetAnimationName() const
	{
		return m_AnimationName;
	}


	bool AnimationAsset::IsMaskedRootMotion() const
	{
		return m_IsMaskedRootMotion;
	}


	const glm::vec3& AnimationAsset::GetRootTranslationMask() const
	{
		return m_RootTranslationMask;
	}


	float AnimationAsset::GetRootRotationMask() const
	{
		return m_RootRotationMask;
	}


	const Hazel::Animation* AnimationAsset::GetAnimation() const
	{
		Ref<MeshSource> animationSource = AssetManager::GetAsset<MeshSource>(m_AnimationSource);
		Ref<MeshSource> skeletonSource = AssetManager::GetAsset<MeshSource>(m_SkeletonSource);
		if (animationSource && skeletonSource && skeletonSource->GetSkeleton())
		{
			return animationSource->GetAnimation(m_AnimationName, *skeletonSource->GetSkeleton(), m_IsMaskedRootMotion, m_RootTranslationMask, m_RootRotationMask);
		}
		return nullptr;
	}


	void AnimationAsset::OnDependencyUpdated(AssetHandle)
	{
		//Project::GetAssetManager()->ReloadDataAsync(Handle);
	}

}
