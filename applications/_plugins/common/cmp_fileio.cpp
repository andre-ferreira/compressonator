// AMD AMDUtils code
//
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
//
// Major Code based on Header-only tiny glTF 2.0 loader and serializer.
// The MIT License (MIT)
//
// Copyright (c) 2015 - 2017 Syoyo Fujita, Aurélien Chatelain and many
// contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "cmp_fileio.h"

#ifdef _WIN32
#include "windows.h"
#include <stdio.h>
#include <direct.h>
#include <iostream>
#include <iterator>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wordexp.h>
#endif

#ifdef _CMP_CPP17_  // Build code using std::c++17
#include <filesystem>
namespace sfs = std::filesystem;
#else
#include <filesystem>
namespace sfs = std::filesystem;
#endif

#include <algorithm>

bool CMP_DirExists(const std::string& abs_dir)
{
    if (sfs::exists(abs_dir))
        return sfs::is_directory(abs_dir);

    return (false);
}

bool CMP_FileExists(const std::string& abs_filename)
{
    return sfs::exists(abs_filename);
}

bool CMP_CreateDir(std::string sPath)
{
    bool success = sfs::create_directory(sfs::absolute(sPath));
    return (success);
}

std::string CMP_ExpandFilePath(const std::string& filepath)
{
#ifdef _WIN32
    DWORD len = ExpandEnvironmentStringsA(filepath.c_str(), NULL, 0);
    char* str = new char[len];
    ExpandEnvironmentStringsA(filepath.c_str(), str, len);

    std::string s(str);

    delete[] str;

    return s;
#else

#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__)
    // no expansion
    std::string s = filepath;
#else
    std::string s;
    wordexp_t   p;

    if (filepath.empty())
    {
        return "";
    }

    // char** w;
    int ret = wordexp(filepath.c_str(), &p, 0);
    if (ret)
    {
        // err
        s = filepath;
        return s;
    }

    // Use first element only.
    if (p.we_wordv)
    {
        s = std::string(p.we_wordv[0]);
        wordfree(&p);
    }
    else
    {
        s = filepath;
    }

#endif

    return s;
#endif
}

std::string CMP_JoinPath(const std::string& path0, const std::string& path1)
{
    if (path0.empty())
    {
        return path1;
    }
    else
    {
        // check '/'
        char lastChar = *path0.rbegin();
        if (lastChar != '/')
        {
            return path0 + std::string("/") + path1;
        }
        else
        {
            return path0 + path1;
        }
    }
}

std::string CMP_FindFile(const std::vector<std::string>& paths, const std::string& filepath)
{
    for (size_t i = 0; i < paths.size(); i++)
    {
        std::string absPath = CMP_ExpandFilePath(CMP_JoinPath(paths[i], filepath));
        if (CMP_FileExists(absPath))
        {
            return absPath;
        }
    }

    return std::string();
}

std::string CMP_GetFilePathExtension(const std::string& FileName)
{
    if (FileName.find_last_of(".") != std::string::npos)
        return FileName.substr(FileName.find_last_of(".") + 1);
    return "";
}

std::string CMP_GetBaseDir(const std::string& srcfileDirpath)
{
    if (srcfileDirpath.find_last_of(FILE_SPLIT_PATH) != std::string::npos)
        return srcfileDirpath.substr(0, srcfileDirpath.find_last_of(FILE_SPLIT_PATH));
    return "";
}

std::string CMP_GetFileName(const std::string& srcfileNamepath)
{
    int pos = (int)srcfileNamepath.find_last_of(FILE_SPLIT_PATH);
    return srcfileNamepath.substr(pos + 1);
}

bool CMP_IsHidden(const std::string& fullpath)
{
#ifdef _WIN32
    bool  IsHidden = false;
    DWORD Result   = GetFileAttributesA(fullpath.c_str());
    if (Result != 0xFFFFFFFF)
    {
        IsHidden = !!(Result & FILE_ATTRIBUTE_HIDDEN);
    }

    return IsHidden;
#else
    sfs::path path(fullpath);
    if (path.filename().string().find(".") == 0)
        return true;

    return false;
#endif
}

using recursive_directory_iterator = sfs::recursive_directory_iterator;

void CMP_GetAllDirFilesList(const std::string& directory, std::vector<std::string>& files, std::string filter)
{
    std::string path(directory);
    for (const auto& dirEntry : recursive_directory_iterator(path))
    {
        if (sfs::is_regular_file(dirEntry))
        {
            std::string FileNamePath = dirEntry.path().string();
            // Get the file extension if a file filter is suppiled
            if (filter.length() > 0)
            {
                std::string FileName = CMP_GetFileName(FileNamePath);
                std::string ext      = CMP_GetFilePathExtension(FileName);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
                if (filter.find(ext) != std::string::npos) {
                    //std::cout << FileNamePath << std::endl;
                    files.push_back(FileNamePath);
                }
            }
            else {
                //std::cout << FileNamePath << std::endl;
                files.push_back(FileNamePath);
            }
        }
    }
}

void CMP_GetDirList(const std::string& directory, std::vector<std::string>& files, std::string filter)
{
#ifdef _WIN32
    WIN32_FIND_DATAA data;
    HANDLE           hFind;
    std::string      path(directory);
    std::string      fullpath;
    path.append("\\*");
    if ((hFind = FindFirstFileA(path.c_str(), &data)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            fullpath = directory;
            fullpath.append("\\");
            fullpath.append(data.cFileName);
            if (CMP_PathType(fullpath.c_str()) == CMP_PATH_IS_FILE)
            {
                // check file attribute is not hidden
                bool  IsHidden = false;
                DWORD Result   = GetFileAttributesA(fullpath.c_str());
                if (Result != 0xFFFFFFFF)
                {
                    IsHidden = !!(Result & FILE_ATTRIBUTE_HIDDEN);
                }

                if (!IsHidden)
                {
                    // Get the file extension if a file filter is suppiled
                    if (filter.length() > 0)
                    {
                        std::string FileName(data.cFileName);
                        std::string ext = CMP_GetFilePathExtension(FileName);
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
                        if (filter.find(ext) != std::string::npos)
                            files.push_back(fullpath);
                    }
                    else
                        files.push_back(fullpath);
                }
            }
        } while (FindNextFileA(hFind, &data) != 0);
        FindClose(hFind);
    }
#else
    for (const sfs::directory_entry& entry : sfs::directory_iterator(directory))
    {
        if (CMP_IsHidden(entry.path().string()))
            continue;

        if (filter.length() > 0)
        {
            std::string ext = entry.path().extension();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
            if (filter.find(ext) != std::string::npos)
                files.push_back(entry.path());
        }
        else
        {
            files.push_back(entry.path());
        }
    }
#endif
}

std::string CMP_GetFullPath(std::string file)
{
    return sfs::absolute(file).string();
}

CMP_PATHTYPES CMP_PathType(const char* path)
{
#ifdef _WIN32
    DWORD attrib = GetFileAttributesA(path);
    if (attrib != INVALID_FILE_ATTRIBUTES)
    {
        if (attrib & FILE_ATTRIBUTE_DIRECTORY)
        {
            return CMP_PATHTYPES::CMP_PATH_IS_DIR;
        }
        else if (attrib & FILE_ATTRIBUTE_ARCHIVE)
        {
            return CMP_PATHTYPES::CMP_PATH_IS_FILE;
        }
    }
#else
    // works only if file or dir exists!
    if (sfs::is_directory(path))
    {
        return CMP_PATHTYPES::CMP_PATH_IS_DIR;
    }
    else if (sfs::is_regular_file(path))
    {
        return CMP_PATHTYPES::CMP_PATH_IS_FILE;
    }
#endif

    // a none existant file or dir
    std::string unkn = path;

    std::string basedir  = CMP_GetBaseDir(unkn);
    std::string ext      = CMP_GetFilePathExtension(unkn);
    std::string filename = CMP_GetFileName(unkn);

    // files should have an extension
    if (ext.length() > 0)
    {
        if ((  // we only support a limited relative path upto 2 levels up!
                (basedir.compare(".") == 0) || (basedir.compare("..\\..") == 0) || (basedir.compare("../..") == 0) || (basedir.compare("..") == 0)) &&
            ((ext.compare(0, 1, "\\") == 0) || (ext.compare(0, 1, "/") == 0)))
            return CMP_PATH_IS_DIR;
        return CMP_PATH_IS_FILE;
    }
    else
    {  // must be a folder or file with no extension
        if ((basedir.length() > 0))
            return CMP_PATH_IS_DIR;
        if (filename.length() > 0)
            return CMP_PATH_IS_FILE;
    }

    return CMP_PATHTYPES::CMP_PATH_IS_UNKNOWN;
}
