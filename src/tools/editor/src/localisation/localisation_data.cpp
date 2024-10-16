#include "localisation_data.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/tools/project/project.h"

using namespace Halley;

LocalisationStats& LocalisationStats::operator+=(const LocalisationStats& other)
{
	totalWords += other.totalWords;
	totalKeys += other.totalKeys;
	for (auto& [k, v]: other.keysPerCategory) {
		keysPerCategory[k] += v;
	}
	for (auto& [k, v]: other.wordsPerCategory) {
		wordsPerCategory[k] += v;
	}

	return *this;
}

LocalisationDataValue::LocalisationDataValue(String value)
	: value(std::move(value))
{
	computeHash();
}

void LocalisationDataValue::computeHash()
{
	// TODO
	hash = 0;
}

LocalisationDataEntry::LocalisationDataEntry(String key, String value, String context, String comment)
	: key(std::move(key))
	, context(std::move(context))
	, comment(std::move(comment))
{
	values.push_back(std::move(value));
}

namespace {
	int getWordCount(const String& line)
	{
		const char* delims = " ,;.?![]{}()";
		auto start = delims;
		auto end = delims + strlen(delims);

		bool isInWord = false;
		int count = 0;

		for (auto c: line.cppStr()) {
			const bool isWordCharacter = std::find(start, end, c) == end;
			if (isWordCharacter && !isInWord) {
				++count;
			}
			isInWord = isWordCharacter;
		}

		return count;
	}
}

LocalisationStats LocalisationDataChunk::getStats() const
{
	LocalisationStats result;
	for (const auto& entry: entries) {
		const auto wordCount = getWordCount(entry.values.back().value);
		result.totalKeys++;
		result.keysPerCategory[category]++;
		result.totalWords += wordCount;
		result.wordsPerCategory[category] += wordCount;
	}
	return result;
}

bool LocalisationDataChunk::operator<(const LocalisationDataChunk& other) const
{
	return name < other.name;
}

void LocalisationDataChunk::computeHash()
{
	// TODO
	hash = 0;
}

LocalisationStats LocalisationData::getStats() const
{
	LocalisationStats result;
	for (auto& chunk: chunks) {
		result += chunk.getStats();
	}
	return result;
}

TranslationStats LocalisationData::getTranslationStats(const LocalisationData& original) const
{
	TranslationStats result;
	const auto& origKeys = original.keyHashes;

	for (const auto& keyHash: keyHashes) {
		const auto iter = origKeys.find(keyHash.first);

		if (iter != origKeys.end()) {
			if (iter->second == keyHash.second) {
				result.translatedKeys++;
			} else {
				result.outdatedKeys++;
			}
		}
	}

	return result;
}

LocalisationDataChunk& LocalisationData::getChunk(const String& name)
{
	for (auto& chunk: chunks) {
		if (chunk.name == name) {
			return chunk;
		}
	}
	throw Exception("Chunk not found: " + name, HalleyExceptions::Tools);
}

void LocalisationData::alignWith(const LocalisationData& original)
{
	// Store original entries
	HashMap<String, LocalisationDataEntry> entries;
	for (auto& chunk: chunks) {
		for (auto& entry: chunk.entries) {
			const auto key = entry.key;
			entries[key] = std::move(entry);
		}
	}

	// Copy chunks from original
	chunks = original.chunks;

	// Replace entry data
	for (auto& chunk: chunks) {
		auto origEntries = std::move(chunk.entries);
		chunk.entries.clear();

		for (auto& entry: origEntries) {
			const auto iter = entries.find(entry.key);
			if (iter != entries.end()) {
				chunk.entries.push_back(std::move(iter->second));
			}
		}
		chunk.computeHash();
	}
}

namespace {
	LocalisationDataChunk generateChunk(String name, const ConfigNode& data, const ILocalisationInfoRetriever& infoRetriever)
	{
		LocalisationDataChunk result;
		result.category = infoRetriever.getCategory(name);
		result.name = std::move(name);

		for (const auto& entry: data.asSequence()) {
			String context; // TODO
			String comment; // TODO
			result.entries.emplace_back(entry["key"].asString(), entry["value"].asString(""), std::move(context), std::move(comment));
		}

		result.computeHash();

		return result;
	}
}

LocalisationData LocalisationData::generateFromProject(const I18NLanguage& language, Project& project, const ILocalisationInfoRetriever& infoRetriever)
{
	LocalisationData result;
	result.language = language;

	auto suffix = language.getISOCode();

	const auto& rootPath = project.getAssetsSrcPath() / "config" / "strings";
	for (const auto& assetName: project.getFileSystemCache().enumerateDirectory(rootPath)) {
		if (!assetName.getString().contains(suffix)) {
			continue;
		}

		const auto data = Path::readFile(rootPath / assetName);
		if (!data.empty()) {
			const auto config = YAMLConvert::parseConfig(data, YAMLConvert::ParseOptions{ true });

			for (auto& languageNode: config.getRoot().asSequence()) {
				const auto curLang = I18NLanguage(languageNode["key"].asString());
				if (curLang == language) {
					auto chunkName = assetName.replaceExtension("").getString(false).replaceAll("-" + suffix, "").replaceAll("_" + suffix, "");
					result.chunks.push_back(generateChunk(std::move(chunkName), languageNode["value"], infoRetriever));
				}
			}
		}
	}

	for (const auto& chunk: result.chunks) {
		for (const auto& entry: chunk.entries) {
			const auto& value = entry.values.back();
			result.keyHashes[value.value] = value.hash;
		}
	}

	return result;
}
