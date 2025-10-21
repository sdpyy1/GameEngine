#include "hzpch.h"
#include "AssimpAnimationImporter.h"
#include "Model/Animation.h"
#include <map>
#include <acl/compression/compress.h>
#include <acl/compression/pre_process.h>
#include <acl/compression/track_array.h>
#include <acl/core/ansi_allocator.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <set>
#include <unordered_map>
#include <unordered_set>

namespace Hazel {
	namespace Utils {

		acl::iallocator& GetAnimationAllocator();

		glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix);

		float AngleAroundYAxis(const glm::quat& quat)
		{
			static glm::vec3 xAxis = { 1.0f, 0.0f, 0.0f };
			static glm::vec3 yAxis = { 0.0f, 1.0f, 0.0f };
			auto rotatedOrthogonal = quat * xAxis;
			auto projected = glm::normalize(rotatedOrthogonal - (yAxis * glm::dot(rotatedOrthogonal, yAxis)));
			return acos(glm::dot(xAxis, projected));
		}
	}
	// Blender likes to prefix animation names with the name of the armature. like "Armature001|Walk"
	// Strip off the prefix here.
	// This ensures that if we round-trip an asset through Blender, we can still find the animation by name afterwards.
	// (e.g. the asset might start out with animation name like "Walk", but after Blender round-trip it becomes "Armature001|Walk")
	std::string SanitiseAnimationName(std::string_view animationName)
	{
		std::string name = std::string(animationName);
		if (auto pos = name.find_last_of('|'); pos != std::string::npos)
		{
			name = name.substr(pos + 1);
		}
		return name;
	}
	Scope<Skeleton> AssimpAnimationImporter::ImportSkeleton(const aiScene* scene)
	{
		BoneHierarchy boneHierarchy(scene);
		return boneHierarchy.CreateSkeleton();
	}
	std::vector<std::string> AssimpAnimationImporter::GetAnimationNames(const aiScene* scene)
	{
		std::vector<std::string> animationNames;
		if (scene)
		{
			animationNames.reserve(scene->mNumAnimations);
			for (size_t i = 0; i < scene->mNumAnimations; ++i)
			{
				HZ_CORE_INFO("找到动画: {}", scene->mAnimations[i]->mName.C_Str());
				animationNames.emplace_back(SanitiseAnimationName(scene->mAnimations[i]->mName.C_Str()));
			}
		}
		return animationNames;
	}

	template<typename T> struct KeyFrame
	{
		float FrameTime;
		T Value;
		KeyFrame(const float frameTime, const T& value) : FrameTime(frameTime), Value(value) {}
	};

	struct Channel
	{
		std::vector<KeyFrame<glm::vec3>> Translations;
		std::vector<KeyFrame<glm::quat>> Rotations;
		std::vector<KeyFrame<glm::vec3>> Scales;
		uint32_t Index;
	};
	static std::vector<Channel> ImportChannels(aiAnimation* anim, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask)
	{
		std::vector<Channel> channels;

		std::unordered_map<std::string_view, uint32_t> boneIndices;
		std::unordered_set<uint32_t> rootBoneIndices;
		for (uint32_t i = 0; i < skeleton.GetNumBones(); ++i)
		{
			boneIndices.emplace(skeleton.GetBoneName(i), i + 1);  // 0 is reserved for root motion channel    boneIndices are base=1
			if (skeleton.GetParentBoneIndex(i) == Skeleton::NullIndex)
				rootBoneIndices.emplace(i + 1);
		}

		std::map<uint32_t, aiNodeAnim*> validChannels;
		for (uint32_t channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex)
		{
			aiNodeAnim* nodeAnim = anim->mChannels[channelIndex];
			auto it = boneIndices.find(nodeAnim->mNodeName.C_Str());
			if (it != boneIndices.end())
			{
				validChannels.emplace(it->second, nodeAnim);   // validChannels.first is base=1,  .second is node pointer
			}
		}

		channels.resize(skeleton.GetNumBones() + 1);   // channels is base=1

		// channels don't necessarily have first frame at time zero.
		// We can just generate a dummy key frame, but that can end up looking a bit odd (in particular,
		// looping animations appear to pause for a split second each time they loop and encounter the
		// dummy key frame.
		// So instead, we clip the animation time horizon.
		double firstFrameDelta = DBL_MAX;
		double animationDuration = anim->mDuration;
		for (uint32_t boneIndex = 1; boneIndex < channels.size(); ++boneIndex)
		{
			if (auto validChannel = validChannels.find(boneIndex); validChannel != validChannels.end())
			{
				auto nodeAnim = validChannel->second;
				if (nodeAnim->mNumPositionKeys > 0)
					firstFrameDelta = std::min(firstFrameDelta, nodeAnim->mPositionKeys[0].mTime);

				if (nodeAnim->mNumRotationKeys > 0)
					firstFrameDelta = std::min(firstFrameDelta, nodeAnim->mRotationKeys[0].mTime);

				if (nodeAnim->mNumScalingKeys > 0)
					firstFrameDelta = std::min(firstFrameDelta, nodeAnim->mScalingKeys[0].mTime);
			}
		}

		anim->mDuration -= firstFrameDelta;

		// The rest of the code assumes non-zero animation duration.
		// Enforce that here.
		if (anim->mDuration <= 0.0) {
			anim->mDuration = 1.0;
		}

		for (uint32_t boneIndex = 1; boneIndex < channels.size(); ++boneIndex)
		{
			Channel& channel = channels[boneIndex];
			channel.Index = boneIndex;
			if (auto validChannel = validChannels.find(boneIndex); validChannel != validChannels.end())
			{
				auto nodeAnim = validChannel->second;
				channel.Translations.reserve(nodeAnim->mNumPositionKeys + 2); // +2 because worst case we insert two more keys
				channel.Rotations.reserve(nodeAnim->mNumRotationKeys + 2);
				channel.Scales.reserve(nodeAnim->mNumScalingKeys + 2);


				// Note: There is no need to check for duplicate keys (i.e. multiple keys all at same frame time)
				//       because Assimp throws these out for us
				for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumPositionKeys; ++keyIndex)
				{
					aiVectorKey key = nodeAnim->mPositionKeys[keyIndex];
					float frameTime = std::clamp(static_cast<float>((key.mTime - firstFrameDelta) / anim->mDuration), 0.0f, 1.0f);
					if ((keyIndex == 0) && (frameTime > 0.0f))
					{
						channels[boneIndex].Translations.emplace_back(0.0f, glm::vec3{ static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
					}
					channel.Translations.emplace_back(frameTime, glm::vec3{ static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
				}
				if (channel.Translations.empty())
				{
					HZ_CORE_WARN_TAG("Animation", "No translation track found for bone '{}'", skeleton.GetBoneName(boneIndex - 1));
					channel.Translations = { {0.0f, glm::vec3{0.0f}}, {1.0f, glm::vec3{0.0f}} };
				}
				else if (channel.Translations.back().FrameTime < 1.0f)
				{
					channel.Translations.emplace_back(1.0f, channel.Translations.back().Value);
				}
				for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumRotationKeys; ++keyIndex)
				{
					aiQuatKey key = nodeAnim->mRotationKeys[keyIndex];
					float frameTime = std::clamp(static_cast<float>((key.mTime - firstFrameDelta) / anim->mDuration), 0.0f, 1.0f);
					// WARNING: constructor parameter order for a quat is still WXYZ even if you have defined GLM_FORCE_QUAT_DATA_XYZW
					if ((keyIndex == 0) && (frameTime > 0.0f))
					{
						channel.Rotations.emplace_back(0.0f, glm::quat{ static_cast<float>(key.mValue.w), static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
					}
					channel.Rotations.emplace_back(frameTime, glm::quat{ static_cast<float>(key.mValue.w), static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
					HZ_CORE_ASSERT(fabs(glm::length(channels[boneIndex].Rotations.back().Value) - 1.0f) < 0.00001f);   // check rotations are normalized (I think assimp ensures this, but not 100% sure)
				}
				if (channel.Rotations.empty())
				{
					HZ_CORE_WARN_TAG("Animation", "No rotation track found for bone '{}'", skeleton.GetBoneName(boneIndex - 1));
					channel.Rotations = { {0.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}}, {1.0f, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}} };
				}
				else if (channel.Rotations.back().FrameTime < 1.0f)
				{
					channel.Rotations.emplace_back(1.0f, channel.Rotations.back().Value);
				}
				for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumScalingKeys; ++keyIndex)
				{
					aiVectorKey key = nodeAnim->mScalingKeys[keyIndex];
					float frameTime = std::clamp(static_cast<float>((key.mTime - firstFrameDelta) / anim->mDuration), 0.0f, 1.0f);
					if (keyIndex == 0 && frameTime > 0.0f)
					{
						channel.Scales.emplace_back(0.0f, glm::vec3{ static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
					}
					channel.Scales.emplace_back(frameTime, glm::vec3{ static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z) });
				}
				if (channel.Scales.empty())
				{
					HZ_CORE_WARN_TAG("Animation", "No scale track found for bone '{}'", skeleton.GetBoneName(boneIndex - 1));
					channel.Scales = { {0.0f, glm::vec3{1.0f}}, {1.0f, glm::vec3{1.0f}} };
				}
				else if (channel.Scales.back().FrameTime < 1.0f)
				{
					channel.Scales.emplace_back(1.0f, channels[boneIndex].Scales.back().Value);
				}
			}
			else
			{
				HZ_CORE_WARN_TAG("Animation", "No animation tracks found for bone '{}'", skeleton.GetBoneName(boneIndex - 1));

				auto translation = skeleton.GetBoneTranslations().at(boneIndex - 1);
				auto rotation = skeleton.GetBoneRotations().at(boneIndex - 1);
				auto scale = skeleton.GetBoneScales().at(boneIndex - 1);

				channel.Translations = { {0.0f, translation}, {1.0f, translation} };
				channel.Rotations = { {0.0f, rotation}, {1.0f, rotation} };
				channel.Scales = { {0.0f, scale}, {1.0f, scale} };
			}
		}

		// Create root motion channel.
		// If isMaskedRootMotion is true, then root motion channel is created by filtering components of the first channel.
		// Otherwise root motion channel is copied as-is from the first channel.
		//
		// Root motion is then removed from all "root" channels (so it doesn't get applied twice)

		HZ_CORE_ASSERT(!rootBoneIndices.empty()); // Can't see how this would ever be false!
		HZ_CORE_ASSERT(rootBoneIndices.find(1) != rootBoneIndices.end()); // First bone must be a root!

		Channel& root = channels[0];
		root.Index = 0;
		if (isMaskedRootMotion)
		{
			for (auto& translation : channels[1].Translations)
			{
				root.Translations.emplace_back(translation.FrameTime, translation.Value * rootTranslationMask);
				translation.Value *= (glm::vec3(1.0f) - rootTranslationMask);
				translation.Value += root.Translations.front().Value;
			}
			for (auto& rotation : channels[1].Rotations)
			{
				if (rootRotationMask > 0.0f)
				{
					auto angleY = Utils::AngleAroundYAxis(rotation.Value);
					root.Rotations.emplace_back(rotation.FrameTime, glm::quat{ glm::cos(angleY * 0.5f), glm::vec3{0.0f, 1.0f, 0.0f} *glm::sin(angleY * 0.5f) });
					rotation.Value = glm::conjugate(glm::quat(glm::cos(angleY * 0.5f), glm::vec3{ 0.0f, 1.0f, 0.0f } *glm::sin(angleY * 0.5f))) * rotation.Value;
					rotation.Value *= root.Rotations.front().Value;
				}
				else
				{
					root.Rotations.emplace_back(rotation.FrameTime, glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f });
				}
			}
		}
		else
		{
			root.Translations = channels[1].Translations;
			root.Rotations = channels[1].Rotations;
			channels[1].Translations = { {0.0f, root.Translations.front().Value}, {1.0f, root.Translations.front().Value} };
			channels[1].Rotations = { {0.0f, root.Rotations.front().Value}, {1.0f, root.Rotations.front().Value} };
		}
		root.Scales = { {0.0f, glm::vec3{1.0f}}, {1.0f, glm::vec3{1.0f}} };

		// It is possible that there is more than one "root" bone in the asset.
		// We need to remove the root motion from all of them (otherwise those bones will move twice as fast when root motion is applied)
		for (const auto rootBoneIndex : rootBoneIndices)
		{
			// we already removed root motion from the first bone, above
			if (rootBoneIndex != 1)
			{
				for (auto& translation : channels[rootBoneIndex].Translations)
				{
					// sample root channel at this translation's frametime
					for (size_t rootFrame = 0; rootFrame < root.Translations.size() - 1; ++rootFrame)
					{
						if (root.Translations[rootFrame + 1].FrameTime >= translation.FrameTime)
						{
							const float alpha = (translation.FrameTime - root.Translations[rootFrame].FrameTime) / (root.Translations[rootFrame + 1].FrameTime - root.Translations[rootFrame].FrameTime);
							translation.Value -= glm::mix(root.Translations[rootFrame].Value, root.Translations[rootFrame + 1].Value, alpha);
							translation.Value += root.Translations.front().Value;
							break;
						}
					}
				}

				for (auto& rotation : channels[rootBoneIndex].Rotations)
				{
					// sample root channel at the this rotation's frametime
					for (size_t rootFrame = 0; rootFrame < root.Rotations.size() - 1; ++rootFrame)
					{
						if (root.Rotations[rootFrame + 1].FrameTime >= rotation.FrameTime)
						{
							const float alpha = (rotation.FrameTime - root.Rotations[rootFrame].FrameTime) / (root.Rotations[rootFrame + 1].FrameTime - root.Rotations[rootFrame].FrameTime);
							rotation.Value = glm::normalize(glm::conjugate(glm::slerp(root.Rotations[rootFrame].Value, root.Rotations[rootFrame + 1].Value, alpha)) * rotation.Value);
							rotation.Value *= root.Rotations.front().Value;
							break;
						}
					}
				}
			}
		}
		return channels;
	}
	acl::error_result CompressChannels(const std::vector<Channel>& channels, const float fps, const Skeleton& skeleton, acl::compressed_tracks*& outCompressedTracks)
	{
		acl::iallocator& allocator = Utils::GetAnimationAllocator();
		uint32_t numTracks = static_cast<uint32_t>(channels.size());
		uint32_t numSamples = static_cast<uint32_t>(channels[0].Translations.size());
		acl::track_array_qvvf rawTrackList(allocator, numTracks);

		for (uint32_t i = 0; i < numTracks; ++i)
		{
			acl::track_desc_transformf desc;
			desc.output_index = i;
			desc.parent_index = (i == 0) ? acl::k_invalid_track_index : skeleton.GetParentBoneIndex(i - 1) + 1;  // 0 is root motion channel, 1..numBones are in the skeleton
			desc.precision = 0.0001f;
			desc.shell_distance = 3.0f;

			acl::track_qvvf rawTrack = acl::track_qvvf::make_reserve(desc, allocator, numSamples, fps);
			for (uint32_t j = 0; j < numSamples; ++j)
			{
				const auto& translation = channels[i].Translations[j].Value;
				const auto& rotation = channels[i].Rotations[j].Value;
				const auto& scale = channels[i].Scales[j].Value;

				rawTrack[j].rotation = rtm::quat_set(rotation.x, rotation.y, rotation.z, rotation.w);
				rawTrack[j].translation = rtm::vector_set(translation.x, translation.y, translation.z);
				rawTrack[j].scale = rtm::vector_set(scale.x, scale.y, scale.z);
			}
			rawTrackList[i] = std::move(rawTrack);
		}

		acl::pre_process_settings_t preProcessSettings;
		preProcessSettings.actions = acl::pre_process_actions::recommended;
		preProcessSettings.precision_policy = acl::pre_process_precision_policy::lossy;
		acl::qvvf_transform_error_metric error_metric;
		preProcessSettings.error_metric = &error_metric;
		acl::error_result result = acl::pre_process_track_list(allocator, preProcessSettings, rawTrackList);
		if (result.any())
		{
			return result;
		}

		acl::compression_settings compressSettings = acl::get_default_compression_settings();
		compressSettings.error_metric = &error_metric;
		acl::output_stats stats{ acl::stat_logging::none };
		result = acl::compress_track_list(allocator, rawTrackList, compressSettings, outCompressedTracks, stats);

		return result;
	}

	void SanitizeChannels(std::vector<Channel>& channels)
	{
		uint32_t maxNumFrames = 2; // The rest of the code requires each channel to have at least 2 frames
		for (const auto& channel : channels)
		{
			maxNumFrames = std::max(maxNumFrames, static_cast<uint32_t>(channel.Translations.size()));
			maxNumFrames = std::max(maxNumFrames, static_cast<uint32_t>(channel.Rotations.size()));
			maxNumFrames = std::max(maxNumFrames, static_cast<uint32_t>(channel.Scales.size()));
		}

		float frameInterval = 1.0f / (maxNumFrames - 1);

		// loop over all channels and change them so they all have maxNumFrames frames.
		// add new frames where necessary by interpolating between existing frames.
		for (auto& channel : channels)
		{
			Channel newChannel;

			uint32_t translationIndex = 1;
			newChannel.Translations.reserve(maxNumFrames);
			newChannel.Translations.emplace_back(channel.Translations[0]);
			for (uint32_t i = 1; i < maxNumFrames - 1; ++i)
			{
				float frameTime = i * frameInterval;
				while ((translationIndex < channel.Translations.size()) && (channel.Translations[translationIndex].FrameTime < frameTime))
				{
					++translationIndex;
				}
				const float t = (frameTime - channel.Translations[translationIndex - 1].FrameTime) / (channel.Translations[translationIndex].FrameTime - channel.Translations[translationIndex - 1].FrameTime);
				newChannel.Translations.emplace_back(frameTime, glm::mix(channel.Translations[translationIndex - 1].Value, channel.Translations[translationIndex].Value, t));
			}
			newChannel.Translations.emplace_back(channel.Translations.back());

			uint32_t rotationIndex = 1;
			newChannel.Rotations.reserve(maxNumFrames);
			newChannel.Rotations.emplace_back(channel.Rotations[0]);
			for (uint32_t i = 1; i < maxNumFrames - 1; ++i)
			{
				float frameTime = i * frameInterval;
				while ((rotationIndex < channel.Rotations.size()) && (channel.Rotations[rotationIndex].FrameTime < frameTime))
				{
					++rotationIndex;
				}
				const float t = (frameTime - channel.Rotations[rotationIndex - 1].FrameTime) / (channel.Rotations[rotationIndex].FrameTime - channel.Rotations[rotationIndex - 1].FrameTime);
				newChannel.Rotations.emplace_back(frameTime, glm::slerp(channel.Rotations[rotationIndex - 1].Value, channel.Rotations[rotationIndex].Value, t));
			}
			newChannel.Rotations.emplace_back(channel.Rotations.back());

			uint32_t scaleIndex = 1;
			newChannel.Scales.reserve(maxNumFrames);
			newChannel.Scales.emplace_back(channel.Scales[0]);
			for (uint32_t i = 1; i < maxNumFrames - 1; ++i)
			{
				float frameTime = i * frameInterval;
				while ((scaleIndex < channel.Scales.size()) && (channel.Scales[scaleIndex].FrameTime < frameTime))
				{
					++scaleIndex;
				}
				const float t = (frameTime - channel.Scales[scaleIndex - 1].FrameTime) / (channel.Scales[scaleIndex].FrameTime - channel.Scales[scaleIndex - 1].FrameTime);
				newChannel.Scales.emplace_back(frameTime, glm::mix(channel.Scales[scaleIndex - 1].Value, channel.Scales[scaleIndex].Value, t));
			}
			newChannel.Scales.emplace_back(channel.Scales.back());

			channel.Translations = std::move(newChannel.Translations);
			channel.Rotations = std::move(newChannel.Rotations);
			channel.Scales = std::move(newChannel.Scales);
		}
	}

	Scope<Animation> AssimpAnimationImporter::ImportAnimation(const aiScene* scene, const uint32_t animationIndex, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask)
	{
		if (!scene)
		{
			return nullptr;
		}

		if (animationIndex >= scene->mNumAnimations)
		{
			return nullptr;
		}

		aiAnimation* anim = scene->mAnimations[animationIndex];
		// 导入所有通道
		std::vector<Channel> channels = ImportChannels(anim, skeleton, isMaskedRootMotion, rootTranslationMask, rootRotationMask);
		// 统一化所有的通道，使它们具有相同的关键帧数量
		SanitizeChannels(channels);

		double samplingRate = anim->mTicksPerSecond;
		if (samplingRate < 0.0001)
		{
			samplingRate = 1.0;
		}
		float fps = static_cast<float>(static_cast<double>(channels[0].Translations.size() - 1) * samplingRate / anim->mDuration);

		acl::compressed_tracks* compressedTracks = nullptr;
		acl::error_result result = CompressChannels(channels, fps, skeleton, compressedTracks);
		if (result.any())
		{
			HZ_CORE_ERROR("Failed to compress animation '{0}' with error code {1}", anim->mName.C_Str(), result.c_str());
			return nullptr;
		}

		return CreateScope<Animation>(static_cast<float>(anim->mDuration / samplingRate), static_cast<uint32_t>(channels.size()), compressedTracks);
	}
	uint32_t AssimpAnimationImporter::GetAnimationIndex(const aiScene* scene, const std::string_view animationName)
	{
		for (size_t i = 0; i < scene->mNumAnimations; ++i)
		{
			if (SanitiseAnimationName(scene->mAnimations[i]->mName.C_Str()) == animationName)
			{
				return static_cast<uint32_t>(i);
			}
		}
		return ~0;
	}
}
