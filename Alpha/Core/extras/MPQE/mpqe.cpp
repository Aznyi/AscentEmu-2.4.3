#define _CRT_SECURE_NO_WARNINGS
#define __STORMLIB_SELF__

#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "..\collision\extractor\stormlib\StormLib.h"

namespace fs = std::filesystem;

namespace
{
	struct ArchiveCandidate
	{
		fs::path path;
		std::string name;
		bool isPatch = false;
		bool isLocalePatch = false;
		bool isLocaleArchive = false;
		int patchNumber = 0;
		int priorityBucket = 0;
		std::string localeName;
	};

	struct Options
	{
		bool usePatchFiles = false;
		bool verbose = false;
		bool lowercase = false;
		bool dbcOnly = false;
		bool tbcMode = false;
		bool outDirSet = false;
		fs::path outDir = "MPQOUT";
		std::string locale;
		fs::path inputPath;
		std::string searchGlob;
	};

	std::string ToLowerCopy(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	std::string NormalizeSlashes(std::string value)
	{
		std::replace(value.begin(), value.end(), '/', '\\');
		return value;
	}

	bool WildcardMatchImpl(std::string_view pattern, std::string_view input)
	{
		size_t inputIndex = 0;
		size_t patternIndex = 0;
		size_t checkpointPattern = 0;
		size_t checkpointInput = 0;

		while(inputIndex < input.size() && patternIndex < pattern.size() && pattern[patternIndex] != '*')
		{
			if(pattern[patternIndex] != '?' && pattern[patternIndex] != input[inputIndex])
				return false;

			++inputIndex;
			++patternIndex;
		}

		if(patternIndex == pattern.size())
			return inputIndex == input.size();

		while(inputIndex < input.size())
		{
			if(patternIndex < pattern.size() && pattern[patternIndex] == '*')
			{
				++patternIndex;
				if(patternIndex >= pattern.size())
					return true;

				checkpointPattern = patternIndex;
				checkpointInput = inputIndex + 1;
			}
			else if(patternIndex < pattern.size() && (pattern[patternIndex] == '?' || pattern[patternIndex] == input[inputIndex]))
			{
				++patternIndex;
				++inputIndex;
			}
			else
			{
				patternIndex = checkpointPattern;
				inputIndex = checkpointInput++;
			}
		}

		while(patternIndex < pattern.size() && pattern[patternIndex] == '*')
			++patternIndex;

		return patternIndex == pattern.size();
	}

	bool WildcardMatch(const std::string& pattern, const std::string& value, bool caseSensitive)
	{
		if(caseSensitive)
			return WildcardMatchImpl(pattern, value);

		return WildcardMatchImpl(ToLowerCopy(pattern), ToLowerCopy(value));
	}

	std::string BytesToText(uint64_t bytes)
	{
		std::ostringstream stream;
		if(bytes < 1024)
		{
			stream << bytes << 'B';
			return stream.str();
		}

		if(bytes < (1024ULL * 1024ULL))
		{
			stream << std::llround(static_cast<long double>(bytes) / 1024.0L) << 'K';
			return stream.str();
		}

		if(bytes < (1024ULL * 1024ULL * 1024ULL))
		{
			stream.setf(std::ios::fixed, std::ios::floatfield);
			stream.precision(1);
			stream << (static_cast<long double>(bytes) / 1024.0L / 1024.0L) << 'M';
			return stream.str();
		}

		stream.setf(std::ios::fixed, std::ios::floatfield);
		stream.precision(2);
		stream << (static_cast<long double>(bytes) / 1024.0L / 1024.0L / 1024.0L) << 'G';
		return stream.str();
	}

	void PrintHelp()
	{
		std::cout << "Extracts files from MoPAQ archives with deterministic TBC patch ordering.\n\n";
		std::cout << "MPQE [options] <MPQfile|WoWDir|DataDir> [glob]\n\n";
		std::cout << "Options:\n";
		std::cout << " /p\t\tUse patch MPQ archives if available\n";
		std::cout << " /tbc\t\tBuild a TBC archive stack from a WoW client/Data directory\n";
		std::cout << " /dbc\t\tExtract DBCs from the TBC archive stack into DBC\\\\ by default\n";
		std::cout << " /locale <name>\tRestrict locale archives (for example enGB)\n";
		std::cout << " /d <directory>\tSet output directory (default: MPQOUT or DBC for /dbc)\n";
		std::cout << " /v\t\tEnable verbose output\n";
		std::cout << " /l\t\tUse lowercase filenames\n";
	}

	int ExtractPatchNumber(const std::string& lowerName)
	{
		const size_t dashIndex = lowerName.find_last_of('-');
		const size_t dotIndex = lowerName.find_last_of('.');
		if(dashIndex != std::string::npos && dotIndex != std::string::npos && dotIndex > dashIndex)
		{
			const std::string suffix = lowerName.substr(dashIndex + 1, dotIndex - dashIndex - 1);
			try
			{
				return std::stoi(suffix);
			}
			catch(...)
			{
			}
		}

		return 1;
	}

	std::string ExtractLocaleName(const fs::path& archivePath, const fs::path& dataRoot)
	{
		std::error_code ec;
		const fs::path relative = fs::relative(archivePath, dataRoot, ec);
		if(ec)
			return std::string();

		auto it = relative.begin();
		if(it == relative.end())
			return std::string();

		const std::string firstPart = it->string();
		if(firstPart.size() == 4)
			return firstPart;

		return std::string();
	}

	int GetArchivePriorityBucket(const std::string& lowerName, const ArchiveCandidate& candidate)
	{
		if(candidate.isPatch)
		{
			if(candidate.isLocalePatch)
				return 2000 + candidate.patchNumber;
			return 1000 + candidate.patchNumber;
		}

		if(lowerName == "common.mpq")
			return 10;
		if(lowerName == "common-2.mpq")
			return 20;
		if(lowerName == "expansion.mpq")
			return 30;
		if(lowerName.rfind("locale-", 0) == 0)
			return 40;
		if(lowerName.rfind("speech-", 0) == 0)
			return 50;
		if(lowerName.rfind("expansion-locale-", 0) == 0)
			return 60;
		if(lowerName.rfind("expansion-speech-", 0) == 0)
			return 70;
		if(candidate.isLocaleArchive)
			return 80;
		return 90;
	}

	ArchiveCandidate BuildArchiveCandidate(const fs::path& archivePath, const fs::path& dataRoot)
	{
		ArchiveCandidate candidate;
		candidate.path = archivePath;
		candidate.name = archivePath.filename().string();
		const std::string lowerName = ToLowerCopy(candidate.name);
		candidate.isPatch = lowerName.rfind("patch", 0) == 0;
		candidate.patchNumber = ExtractPatchNumber(lowerName);
		candidate.localeName = ExtractLocaleName(archivePath, dataRoot);
		candidate.isLocaleArchive = !candidate.localeName.empty();
		candidate.isLocalePatch = candidate.isPatch && candidate.isLocaleArchive;
		candidate.priorityBucket = GetArchivePriorityBucket(lowerName, candidate);
		return candidate;
	}

	bool ShouldIncludeArchive(const Options& options, const ArchiveCandidate& candidate)
	{
		if(!options.locale.empty() && candidate.isLocaleArchive && _stricmp(candidate.localeName.c_str(), options.locale.c_str()) != 0)
			return false;

		if(candidate.isPatch)
			return options.usePatchFiles || options.tbcMode || options.dbcOnly;

		return true;
	}

	std::vector<fs::path> BuildArchivePlan(const Options& options, const fs::path& dataDir)
	{
		std::vector<ArchiveCandidate> candidates;
		for(const auto& entry : fs::recursive_directory_iterator(dataDir))
		{
			if(!entry.is_regular_file())
				continue;

			if(ToLowerCopy(entry.path().extension().string()) != ".mpq")
				continue;

			candidates.push_back(BuildArchiveCandidate(entry.path(), dataDir));
		}

		std::sort(candidates.begin(), candidates.end(), [](const ArchiveCandidate& left, const ArchiveCandidate& right)
		{
			if(left.priorityBucket != right.priorityBucket)
				return left.priorityBucket < right.priorityBucket;

			return _stricmp(left.path.string().c_str(), right.path.string().c_str()) < 0;
		});

		std::vector<fs::path> archives;
		for(const ArchiveCandidate& candidate : candidates)
		{
			if(ShouldIncludeArchive(options, candidate))
				archives.push_back(candidate.path);
		}

		return archives;
	}

	std::vector<fs::path> BuildPatchArchiveList(const fs::path& directory)
	{
		std::vector<fs::path> patches;
		for(const auto& entry : fs::directory_iterator(directory))
		{
			if(!entry.is_regular_file())
				continue;

			const std::string lowerName = ToLowerCopy(entry.path().filename().string());
			if(lowerName.rfind("patch", 0) == 0 && ToLowerCopy(entry.path().extension().string()) == ".mpq")
				patches.push_back(entry.path());
		}

		std::sort(patches.begin(), patches.end(), [](const fs::path& left, const fs::path& right)
		{
			const std::string leftName = ToLowerCopy(left.filename().string());
			const std::string rightName = ToLowerCopy(right.filename().string());
			const int leftKey = ExtractPatchNumber(leftName);
			const int rightKey = ExtractPatchNumber(rightName);
			if(leftKey != rightKey)
				return leftKey < rightKey;

			return _stricmp(leftName.c_str(), rightName.c_str()) < 0;
		});

		return patches;
	}

	fs::path ResolveDataDirectory(const fs::path& inputPath)
	{
		if(_stricmp(inputPath.filename().string().c_str(), "Data") == 0)
			return inputPath;

		const fs::path child = inputPath / "Data";
		if(fs::is_directory(child))
			return child;

		return fs::path();
	}

	bool ReadArchiveFile(HANDLE archiveHandle, const std::string& archiveFileName, std::vector<char>& outBuffer)
	{
		HANDLE fileHandle = nullptr;
		if(!SFileOpenFileEx(archiveHandle, archiveFileName.c_str(), SFILE_OPEN_FROM_MPQ, &fileHandle))
			return false;

		DWORD highSize = 0;
		const DWORD lowSize = SFileGetFileSize(fileHandle, &highSize);
		if(lowSize == 0 && highSize == 0)
		{
			SFileCloseFile(fileHandle);
			outBuffer.clear();
			return true;
		}

		if(highSize != 0)
		{
			SFileCloseFile(fileHandle);
			return false;
		}

		outBuffer.resize(lowSize);
		DWORD bytesRead = 0;
		const BOOL readOk = SFileReadFile(fileHandle, outBuffer.data(), lowSize, &bytesRead, nullptr);
		SFileCloseFile(fileHandle);
		if(!readOk || bytesRead != lowSize)
			return false;

		return true;
	}

	std::string GetOutputRelativePath(const Options& options, const std::string& archiveFileName)
	{
		std::string normalized = NormalizeSlashes(archiveFileName);
		if(options.dbcOnly)
		{
			const std::string lower = ToLowerCopy(normalized);
			if(lower.rfind("dbfilesclient\\", 0) == 0)
				normalized = normalized.substr(std::string("DBFilesClient\\").size());
			else if(lower.rfind("dbc\\", 0) == 0)
				normalized = normalized.substr(4);
		}

		if(options.lowercase)
			normalized = ToLowerCopy(normalized);

		return normalized;
	}

	bool EnsureParentDirectory(const fs::path& filePath)
	{
		const fs::path parent = filePath.parent_path();
		if(parent.empty())
			return true;

		std::error_code ec;
		fs::create_directories(parent, ec);
		return !ec;
	}

	bool ExtractSingleFile(
		const Options& options,
		const fs::path& archivePath,
		HANDLE archiveHandle,
		const std::string& archiveFileName,
		std::set<std::string>& extractedFiles)
	{
		std::vector<char> buffer;
		if(!ReadArchiveFile(archiveHandle, archiveFileName, buffer))
		{
			std::cout << "Error: Could not read " << archiveFileName << " in " << archivePath.string() << '\n';
			return false;
		}

		if(buffer.empty())
		{
			if(options.verbose)
				std::cout << "Skipping: " << archiveFileName << " (NULL)\n";
			return true;
		}

		const std::string outputRelativePath = GetOutputRelativePath(options, archiveFileName);
		const fs::path outputPath = options.outDir / fs::path(outputRelativePath);
		if(!EnsureParentDirectory(outputPath))
		{
			std::cout << "Error: Could not create output directory for " << outputPath.string() << '\n';
			return false;
		}

		std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
		if(!output)
		{
			std::cout << "Error: Could not open " << outputPath.string() << " for writing\n";
			return false;
		}

		output.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		if(!output)
		{
			std::cout << "Error: Could not write " << outputPath.string() << '\n';
			return false;
		}

		extractedFiles.insert(ToLowerCopy(outputRelativePath));
		if(options.verbose)
			std::cout << "Extracted: " << outputRelativePath << " (" << BytesToText(buffer.size()) << ") <- " << archivePath.filename().string() << '\n';

		return true;
	}

	bool ExtractArchive(const Options& options, const fs::path& archivePath, std::set<std::string>& extractedFiles)
	{
		std::cout << "Extracting from " << archivePath.string() << '\n';

		HANDLE archiveHandle = nullptr;
		if(!SFileOpenArchive(archivePath.string().c_str(), 0, 0, &archiveHandle))
		{
			std::cout << "Error: Could not open " << archivePath.string() << '\n';
			return false;
		}

		std::vector<char> listFileBuffer;
		if(!ReadArchiveFile(archiveHandle, LISTFILE_NAME, listFileBuffer))
		{
			SFileCloseArchive(archiveHandle);
			std::cout << "Error: Could not find (listfile) in " << archivePath.string() << '\n';
			return false;
		}

		const std::string listContent(listFileBuffer.begin(), listFileBuffer.end());
		std::stringstream listStream(listContent);
		std::string entry;
		while(std::getline(listStream, entry))
		{
			entry.erase(std::remove(entry.begin(), entry.end(), '\r'), entry.end());
			if(entry.empty())
				continue;

			if(!options.searchGlob.empty() && !WildcardMatch(options.searchGlob, entry, false))
				continue;

			ExtractSingleFile(options, archivePath, archiveHandle, entry, extractedFiles);
		}

		SFileCloseArchive(archiveHandle);
		return true;
	}

	bool ParseArguments(int argc, char** argv, Options& options)
	{
		for(int index = 1; index < argc; ++index)
		{
			const std::string arg = argv[index];
			if(!arg.empty() && arg[0] == '/')
			{
				const std::string opt = ToLowerCopy(arg);
				if(opt == "/p")
				{
					std::cout << "Using patch MPQ archives if available: Enabled\n";
					options.usePatchFiles = true;
				}
				else if(opt == "/l")
				{
					std::cout << "Lowercase output: Enabled\n";
					options.lowercase = true;
				}
				else if(opt == "/v")
				{
					std::cout << "Verbose output: Enabled\n";
					options.verbose = true;
				}
				else if(opt == "/dbc")
				{
					std::cout << "DBC-only extraction mode: Enabled\n";
					options.dbcOnly = true;
					options.tbcMode = true;
					options.usePatchFiles = true;
					if(options.searchGlob.empty())
						options.searchGlob = "DBFilesClient\\*.dbc";
					if(!options.outDirSet)
						options.outDir = "DBC";
				}
				else if(opt == "/tbc")
				{
					std::cout << "TBC archive stack mode: Enabled\n";
					options.tbcMode = true;
				}
				else if(opt == "/d")
				{
					if(index + 1 >= argc)
					{
						std::cout << "Fatal: No output directory specified for /d\n";
						return false;
					}

					options.outDir = argv[++index];
					options.outDirSet = true;
					std::cout << "Output directory: " << options.outDir.string() << '\n';
				}
				else if(opt == "/locale")
				{
					if(index + 1 >= argc)
					{
						std::cout << "Fatal: No locale specified for /locale\n";
						return false;
					}

					options.locale = argv[++index];
					std::cout << "Locale filter: " << options.locale << '\n';
				}
				else
				{
					std::cout << "Fatal: Unknown option " << arg << '\n';
					return false;
				}
			}
			else
			{
				if(options.inputPath.empty())
					options.inputPath = arg;
				else
					options.searchGlob = arg;
			}
		}

		if(options.inputPath.empty())
		{
			std::cout << "Fatal: Did not provide an MPQ file or WoW client directory.\n";
			return false;
		}

		return true;
	}
}

int main(int argc, char** argv)
{
	std::cout << "MPQ-Extractor v2.0 by WRS and Ascent\n";

	if(argc == 1)
	{
		PrintHelp();
		return 0;
	}

	Options options;
	if(!ParseArguments(argc, argv, options))
	{
		PrintHelp();
		return 1;
	}

	std::set<std::string> extractedFiles;
	try
	{
		if(fs::is_regular_file(options.inputPath))
		{
			std::vector<fs::path> archives;
			if(options.usePatchFiles)
			{
				const std::vector<fs::path> patches = BuildPatchArchiveList(options.inputPath.parent_path());
				archives.insert(archives.end(), patches.begin(), patches.end());
			}

			archives.push_back(options.inputPath);
			for(const fs::path& archivePath : archives)
				ExtractArchive(options, archivePath, extractedFiles);
		}
		else if(fs::is_directory(options.inputPath))
		{
			const fs::path dataDir = ResolveDataDirectory(options.inputPath);
			if(dataDir.empty())
			{
				std::cout << "Fatal: Could not find a Data directory under " << options.inputPath.string() << '\n';
				return 1;
			}

			const std::vector<fs::path> archives = BuildArchivePlan(options, dataDir);
			if(archives.empty())
			{
				std::cout << "Fatal: No MPQ archives found for extraction under " << dataDir.string() << '\n';
				return 1;
			}

			std::cout << "Archive plan (" << archives.size() << " files):\n";
			for(const fs::path& archivePath : archives)
				std::cout << "  " << archivePath.string() << '\n';

			for(const fs::path& archivePath : archives)
				ExtractArchive(options, archivePath, extractedFiles);
		}
		else
		{
			std::cout << "Fatal: Could not locate input path " << options.inputPath.string() << '\n';
			return 1;
		}
	}
	catch(const std::exception& ex)
	{
		std::cout << "Fatal: " << ex.what() << '\n';
		return 1;
	}

	std::cout << "Extraction complete. Files written: " << extractedFiles.size() << '\n';
	return 0;
}
