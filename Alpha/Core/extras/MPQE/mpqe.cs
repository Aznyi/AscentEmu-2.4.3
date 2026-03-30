/*
 * MPQ Extractor ( SFmpq.dll wrapper )
 * Copyright (C) 2005-2007 WRS <thewrs@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

using System;
using System.IO;
using System.Collections;
using SFmpqapi;

namespace MPQE
{
	class ArchiveCandidate
	{
		public string Path;
		public string Name;
		public bool IsPatch;
		public bool IsLocalePatch;
		public bool IsLocaleArchive;
		public int PatchNumber;
		public int PriorityBucket;
		public string LocaleName;
	}

	class MPQE
	{
		private string version = "1.5";
		private int hMPQ = 0;
		private int hFile = 0;
		private int FileSize = 0;
		private int FileRead = 0;

		private bool option_usepatchfiles = false;
		private bool option_verbose = false;
		private string option_outdir = "MPQOUT";
		private bool option_outdirSet = false;
		private bool option_lowercase = false;
		private bool option_dbcOnly = false;
		private bool option_tbcMode = false;
		private string option_locale = null;
		private string option_inputPath = null;
		private string option_searchglob = null;

		private Hashtable extractedFiles = new Hashtable(StringComparer.OrdinalIgnoreCase);

		[STAThread]
		static void Main(string[] args)
		{
			MPQE mpqe = new MPQE();
			Console.Write("MPQ-Extractor v{0} by WRS", mpqe.version);
			try
			{
				Console.WriteLine(" powered by {0}", SFmpq.MpqGetVersionString());
			}
			catch(Exception e)
			{
				Console.WriteLine();
				Console.WriteLine("Fatal: Failed to initialize SFmpq.");
				Console.WriteLine("Executable directory: {0}", AppDomain.CurrentDomain.BaseDirectory);
				Console.WriteLine("Expected native DLL: {0}", Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "SFmpq.dll"));
				Console.WriteLine("Exception: {0}: {1}", e.GetType().FullName, e.Message);
				if(e.InnerException != null)
					Console.WriteLine("Inner exception: {0}: {1}", e.InnerException.GetType().FullName, e.InnerException.Message);
				return;
			}

			Console.WriteLine();
			if(args.Length == 0)
			{
				mpqe.helper();
				return;
			}

			for(int i = 0; i < args.Length; ++i)
			{
				if(args[i].StartsWith("/"))
				{
					string opt = args[i].ToLower();
					switch(opt)
					{
						case "/p":
							Console.WriteLine("Using patch MPQ archives if available: Enabled");
							mpqe.option_usepatchfiles = true;
							break;
						case "/l":
							Console.WriteLine("Lowercase output: Enabled");
							mpqe.option_lowercase = true;
							break;
						case "/v":
							Console.WriteLine("Verbose output: Enabled");
							mpqe.option_verbose = true;
							break;
						case "/dbc":
							Console.WriteLine("DBC-only extraction mode: Enabled");
							mpqe.option_dbcOnly = true;
							mpqe.option_tbcMode = true;
							mpqe.option_usepatchfiles = true;
							if(mpqe.option_searchglob == null)
								mpqe.option_searchglob = "DBFilesClient\\*.dbc";
							if(!mpqe.option_outdirSet)
								mpqe.option_outdir = "DBC";
							break;
						case "/tbc":
							Console.WriteLine("TBC archive stack mode: Enabled");
							mpqe.option_tbcMode = true;
							break;
						case "/d":
							if(i + 1 >= args.Length)
							{
								Console.WriteLine("Fatal: No output directory specified for /d");
								mpqe.helper();
								return;
							}
							++i;
							Console.WriteLine("Output directory: {0}", args[i]);
							mpqe.option_outdir = args[i];
							mpqe.option_outdirSet = true;
							break;
						case "/locale":
							if(i + 1 >= args.Length)
							{
								Console.WriteLine("Fatal: No locale specified for /locale");
								mpqe.helper();
								return;
							}
							++i;
							Console.WriteLine("Locale filter: {0}", args[i]);
							mpqe.option_locale = args[i];
							break;
						default:
							Console.WriteLine("Fatal: Unknown option {0}", args[i]);
							mpqe.helper();
							return;
					}
				}
				else
				{
					if(mpqe.option_inputPath == null)
						mpqe.option_inputPath = args[i];
					else
						mpqe.option_searchglob = args[i];
				}
			}

			if(mpqe.option_inputPath == null)
			{
				Console.WriteLine("Fatal: Did not provide an MPQ file or WoW client directory.");
				mpqe.helper();
				return;
			}

			mpqe.worker();
		}

		private void helper()
		{
			Console.WriteLine("Extracts files from MoPAQ archives with deterministic TBC patch ordering.");
			Console.WriteLine();
			Console.WriteLine("MPQE [options] <MPQfile|WoWDir|DataDir> [glob]");
			Console.WriteLine();
			Console.WriteLine("Options:");
			Console.WriteLine(" /p\t\tUse patch MPQ archives if available");
			Console.WriteLine(" /tbc\t\tBuild a TBC archive stack from a WoW client/Data directory");
			Console.WriteLine(" /dbc\t\tExtract DBCs from the TBC archive stack into DBC\\ by default");
			Console.WriteLine(" /locale <name>\tRestrict locale archives (for example enUS)");
			Console.WriteLine(" /d <directory>\tSet output directory ( default: MPQOUT or DBC for /dbc )");
			Console.WriteLine(" /v\t\tEnable verbose output");
			Console.WriteLine(" /l\t\tUse lowercase filenames");
		}

		private void worker()
		{
			if(File.Exists(option_inputPath))
			{
				ProcessSingleArchiveInput(new FileInfo(option_inputPath));
				return;
			}

			if(Directory.Exists(option_inputPath))
			{
				ProcessDirectoryInput(new DirectoryInfo(option_inputPath));
				return;
			}

			Console.WriteLine("Fatal: Could not locate input path {0}", option_inputPath);
		}

		private void ProcessSingleArchiveInput(FileInfo archiveFile)
		{
			ArrayList archives = new ArrayList();
			if(option_usepatchfiles)
				AddPatchArchivesFromDirectory(archiveFile.DirectoryName, archives);

			archives.Add(archiveFile.FullName);
			ExtractArchives(archives);
		}

		private void ProcessDirectoryInput(DirectoryInfo inputDir)
		{
			DirectoryInfo dataDir = ResolveDataDirectory(inputDir);
			if(dataDir == null)
			{
				Console.WriteLine("Fatal: Could not find a Data directory under {0}", inputDir.FullName);
				return;
			}

			ArrayList archives = BuildArchivePlan(dataDir);
			if(archives.Count == 0)
			{
				Console.WriteLine("Fatal: No MPQ archives found for extraction under {0}", dataDir.FullName);
				return;
			}

			Console.WriteLine("Archive plan ({0} files):", archives.Count);
			for(int i = 0; i < archives.Count; ++i)
				Console.WriteLine("  {0}", archives[i]);

			ExtractArchives(archives);
		}

		private DirectoryInfo ResolveDataDirectory(DirectoryInfo inputDir)
		{
			if(String.Compare(inputDir.Name, "Data", true) == 0)
				return inputDir;

			DirectoryInfo child = new DirectoryInfo(Path.Combine(inputDir.FullName, "Data"));
			if(child.Exists)
				return child;

			return null;
		}

		private ArrayList BuildArchivePlan(DirectoryInfo dataDir)
		{
			ArrayList candidates = DiscoverArchives(dataDir);
			candidates.Sort(new IComparerArchiveCandidate());

			ArrayList archives = new ArrayList();
			for(int i = 0; i < candidates.Count; ++i)
			{
				ArchiveCandidate candidate = (ArchiveCandidate)candidates[i];
				if(ShouldIncludeArchive(candidate))
					archives.Add(candidate.Path);
			}

			return archives;
		}

		private ArrayList DiscoverArchives(DirectoryInfo dataDir)
		{
			ArrayList candidates = new ArrayList();
			string[] allMpqs = Directory.GetFiles(dataDir.FullName, "*.MPQ", SearchOption.AllDirectories);
			for(int i = 0; i < allMpqs.Length; ++i)
			{
				ArchiveCandidate candidate = BuildArchiveCandidate(allMpqs[i], dataDir.FullName);
				if(candidate != null)
					candidates.Add(candidate);
			}

			return candidates;
		}

		private ArchiveCandidate BuildArchiveCandidate(string archivePath, string dataRoot)
		{
			string name = Path.GetFileName(archivePath);
			string lowerName = name.ToLower();

			ArchiveCandidate candidate = new ArchiveCandidate();
			candidate.Path = archivePath;
			candidate.Name = name;
			candidate.IsPatch = lowerName.StartsWith("patch");
			candidate.PatchNumber = ExtractPatchNumber(lowerName);
			candidate.LocaleName = ExtractLocaleName(archivePath, dataRoot);
			candidate.IsLocaleArchive = candidate.LocaleName != null;
			candidate.IsLocalePatch = candidate.IsPatch && candidate.IsLocaleArchive;
			candidate.PriorityBucket = GetArchivePriorityBucket(lowerName, candidate);
			return candidate;
		}

		private bool ShouldIncludeArchive(ArchiveCandidate candidate)
		{
			if(option_locale != null && candidate.IsLocaleArchive &&
				String.Compare(candidate.LocaleName, option_locale, true) != 0)
				return false;

			if(candidate.IsPatch)
				return option_usepatchfiles || option_tbcMode || option_dbcOnly;

			if(!option_tbcMode && !option_dbcOnly)
				return true;

			return true;
		}

		private int GetArchivePriorityBucket(string lowerName, ArchiveCandidate candidate)
		{
			if(candidate.IsPatch)
			{
				if(candidate.IsLocalePatch)
					return 2000 + candidate.PatchNumber;
				return 1000 + candidate.PatchNumber;
			}

			if(lowerName == "common.mpq")
				return 10;
			if(lowerName == "common-2.mpq")
				return 20;
			if(lowerName == "expansion.mpq")
				return 30;

			if(lowerName.StartsWith("locale-"))
				return 40;
			if(lowerName.StartsWith("speech-"))
				return 50;
			if(lowerName.StartsWith("expansion-locale-"))
				return 60;
			if(lowerName.StartsWith("expansion-speech-"))
				return 70;

			if(candidate.IsLocaleArchive)
				return 80;
			return 90;
		}

		private int ExtractPatchNumber(string lowerName)
		{
			int dashIndex = lowerName.LastIndexOf('-');
			int dotIndex = lowerName.LastIndexOf('.');
			if(dashIndex >= 0 && dotIndex > dashIndex)
			{
				string suffix = lowerName.Substring(dashIndex + 1, dotIndex - dashIndex - 1);
				int parsed;
				if(Int32.TryParse(suffix, out parsed))
					return parsed;
			}

			return 1;
		}

		private string ExtractLocaleName(string archivePath, string dataRoot)
		{
			string relative = archivePath.Substring(dataRoot.Length).TrimStart(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
			string[] parts = relative.Split(Path.DirectorySeparatorChar);
			if(parts.Length >= 2 && parts[0].Length == 4)
				return parts[0];
			return null;
		}

		private void AddPatchArchivesFromDirectory(string directoryName, ArrayList archives)
		{
			ArrayList patchFiles = new ArrayList();
			string[] files = Directory.GetFiles(directoryName, "patch*.MPQ");
			for(int i = 0; i < files.Length; ++i)
				patchFiles.Add(files[i]);

			patchFiles.Sort(new IComparerPatchFile());
			for(int i = 0; i < patchFiles.Count; ++i)
				archives.Add(patchFiles[i]);
		}

		private void ExtractArchives(ArrayList archives)
		{
			for(int i = 0; i < archives.Count; ++i)
				mpqExtract((string)archives[i]);

			Console.WriteLine("Extraction complete. Files written: {0}", extractedFiles.Count);
		}

		private void mpqExtract(string fileMPQ)
		{
			Console.WriteLine("Extracting from " + fileMPQ);
			if(SFmpq.SFileOpenArchive(fileMPQ, 0, 0, ref hMPQ) != 1)
			{
				Console.WriteLine("Error: Could not open {0}", fileMPQ);
				return;
			}

			if(SFmpq.SFileOpenFileEx(hMPQ, "(listfile)", 0, ref hFile) != 1)
			{
				SFmpq.SFileCloseArchive(hMPQ);
				Console.WriteLine("Error: Could not find (listfile) in " + fileMPQ);
				return;
			}

			byte[] buffer;
			FileSize = SFmpq.SFileGetFileSize(hFile, ref FileSize);
			buffer = new byte[FileSize];
			if(SFmpq.SFileReadFile(hFile, buffer, (uint)FileSize, ref FileRead, IntPtr.Zero) != 1)
			{
				SFmpq.SFileCloseFile(hFile);
				SFmpq.SFileCloseArchive(hMPQ);
				Console.WriteLine("Error: Could not read (listfile) in " + fileMPQ);
				return;
			}

			SFmpq.SFileCloseFile(hFile);
			System.Text.ASCIIEncoding enc = new System.Text.ASCIIEncoding();
			string list = enc.GetString(buffer);
			string[] entries = list.Split('\n');
			for(int i = 0; i < entries.Length; ++i)
			{
				string file = entries[i].Trim();
				if(file.Length == 0)
					continue;

				if(option_searchglob != null && !Match(option_searchglob, file, false))
					continue;

				ExtractFileFromArchive(fileMPQ, file);
			}

			SFmpq.SFileCloseArchive(hMPQ);
		}

		private void ExtractFileFromArchive(string fileMPQ, string archiveFileName)
		{
			if(SFmpq.SFileOpenFileEx(hMPQ, archiveFileName, 0, ref hFile) != 1)
			{
				Console.WriteLine("Error: Could not find " + archiveFileName + " in " + fileMPQ);
				return;
			}

			FileSize = SFmpq.SFileGetFileSize(hFile, ref FileSize);
			if(FileSize == 0)
			{
				SFmpq.SFileCloseFile(hFile);
				if(option_verbose)
					Console.WriteLine("Skipping: {0} (NULL)", archiveFileName);
				return;
			}

			byte[] buffer = new byte[FileSize];
			if(SFmpq.SFileReadFile(hFile, buffer, (uint)FileSize, ref FileRead, IntPtr.Zero) != 1)
			{
				SFmpq.SFileCloseFile(hFile);
				Console.WriteLine("Error: Could not read " + archiveFileName + " in " + fileMPQ);
				return;
			}

			SFmpq.SFileCloseFile(hFile);

			string outputRelativePath = GetOutputRelativePath(archiveFileName);
			string outputPath = Path.Combine(option_outdir, outputRelativePath);
			string outputDir = Path.GetDirectoryName(outputPath);
			if(outputDir != null && outputDir.Length > 0 && !Directory.Exists(outputDir))
				Directory.CreateDirectory(outputDir);

			FileStream fs = new FileStream(outputPath, FileMode.Create, FileAccess.Write);
			fs.Write(buffer, 0, FileSize);
			fs.Flush();
			fs.Close();

			extractedFiles[outputRelativePath] = fileMPQ;
			if(option_verbose)
				Console.WriteLine("Extracted: {0} ({1}) <- {2}", outputRelativePath, bytes2text(FileSize), Path.GetFileName(fileMPQ));
		}

		private string GetOutputRelativePath(string archiveFileName)
		{
			string normalized = archiveFileName.Replace('/', '\\').Trim();
			if(option_dbcOnly)
			{
				string lower = normalized.ToLower();
				if(lower.StartsWith("dbfilesclient\\"))
					normalized = normalized.Substring("DBFilesClient\\".Length);
				else if(lower.StartsWith("dbc\\"))
					normalized = normalized.Substring(4);
			}

			if(option_lowercase)
				normalized = normalized.ToLower();
			return normalized;
		}

		public bool Match(string pattern, string s, bool caseSensitive)
		{
			char[] Wildcards = new char[] { '*', '?' };
			if(!caseSensitive)
			{
				pattern = pattern.ToLower();
				s = s.ToLower();
			}
			if(pattern.IndexOfAny(Wildcards) == -1)
				return (s == pattern);
			int i = 0;
			int j = 0;
			while(i < s.Length && j < pattern.Length && pattern[j] != '*')
			{
				if((pattern[j] != s[i]) && (pattern[j] != '?'))
					return false;
				i++;
				j++;
			}
			if(j == pattern.Length)
				return s.Length == pattern.Length;

			int cp = 0;
			int mp = 0;
			while(i < s.Length)
			{
				if(j < pattern.Length && pattern[j] == '*')
				{
					if((j++) >= pattern.Length)
						return true;
					mp = j;
					cp = i + 1;
				}
				else if(j < pattern.Length && (pattern[j] == s[i] || pattern[j] == '?'))
				{
					j++;
					i++;
				}
				else
				{
					j = mp;
					i = cp++;
				}
			}

			while(j < pattern.Length && pattern[j] == '*')
				j++;
			return j >= pattern.Length;
		}

		public string bytes2text(int bytes)
		{
			if(bytes < 1024) return bytes + "B";
			if(bytes < 1024 * 1024) return Math.Round((decimal)bytes / 1024, 0) + "K";
			if(bytes < 1024 * 1024 * 1024) return Math.Round((decimal)bytes / 1024 / 1024, 1) + "M";
			return "moo";
		}
	}

	class IComparerArchiveCandidate : IComparer
	{
		public int Compare(object x, object y)
		{
			ArchiveCandidate a = (ArchiveCandidate)x;
			ArchiveCandidate b = (ArchiveCandidate)y;

			if(a.PriorityBucket != b.PriorityBucket)
				return a.PriorityBucket.CompareTo(b.PriorityBucket);

			return String.Compare(a.Path, b.Path, true);
		}
	}

	class IComparerPatchFile : IComparer
	{
		public int Compare(object x, object y)
		{
			string ax = Path.GetFileName((string)x).ToLower();
			string ay = Path.GetFileName((string)y).ToLower();
			int nx = GetPatchSortKey(ax);
			int ny = GetPatchSortKey(ay);
			if(nx != ny)
				return nx.CompareTo(ny);
			return String.Compare(ax, ay, true);
		}

		private int GetPatchSortKey(string name)
		{
			int dashIndex = name.LastIndexOf('-');
			int dotIndex = name.LastIndexOf('.');
			if(dashIndex >= 0 && dotIndex > dashIndex)
			{
				string suffix = name.Substring(dashIndex + 1, dotIndex - dashIndex - 1);
				int parsed;
				if(Int32.TryParse(suffix, out parsed))
					return parsed;
			}

			return 1;
		}
	}
}
