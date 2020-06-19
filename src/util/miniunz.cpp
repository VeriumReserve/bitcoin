/*
   Modified from sample part of the MiniZip project
   Copyright (C) 2017 The Verium developers
     Modifications for Verium
   Copyright (C) 2012-2017 Nathan Moinvaziri
     https://github.com/nmoinvaz/minizip
   Copyright (C) 2009-2010 Mathias Svensson
     Modifications for Zip64 support
     http://result42.com
   Copyright (C) 2007-2008 Even Rouault
     Modifications of Unzip for Zip64
   Copyright (C) 1998-2010 Gilles Vollant
     http://www.winimage.com/zLibDll/minizip.html

   This program is distributed under the terms of the same license as zlib.
   See the accompanying COPYING file for the full text of the license.
*/

#include <util/miniunz.h>

#include <logging.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#  define MKDIR(d) _mkdir(d)
#else
#  define MKDIR(d) mkdir(d, 0775)
#endif

#ifdef MAC_OSX
#  define fopen64 fopen
#endif


int is_file_within_path(const fs::path& file_path, const fs::path& dir_path)
{
    boost::filesystem::path file_path_abs = absolute(file_path);
    boost::filesystem::path dir_path_abs = absolute(dir_path);

    int file_len = std::distance(file_path_abs.begin(), file_path_abs.end());
    int dir_len = std::distance(dir_path.begin(), dir_path.end());
    if (dir_len > file_len)
        return 0;

    return std::equal(dir_path.begin(), dir_path.end(), file_path_abs.begin());
}

int zip_extract_currentfile(unzFile uf, const fs::path& root_file_path, const char * allowed_dir)
{
    unz_file_info64 file_info = unz_file_info64();
    FILE* fout = NULL;
    int size_buf = 8192;
    void* buf = NULL;
    int err;
    int errclose;
    char filename_inzip[256] = {0};
    const char *curr_filename = NULL;

    err = unzGetCurrentFileInfo64(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
    if (err != UNZ_OK)
    {
        LogPrintf("error %d with zipfile in unzGetCurrentFileInfo64\n", err);
        return err;
    }

    boost::filesystem::path file_path = root_file_path / filename_inzip;
    boost::filesystem::path bootstrap_dir_path = root_file_path / allowed_dir;

    /* Sanity check to prevent path traversal attacks in case of a malicious zip file */
    if (!is_file_within_path(file_path, bootstrap_dir_path))
    {
        LogPrintf("invalid zipfile: file has invalid directory: %s\n", filename_inzip);
        return UNZ_BADZIPFILE;
    }

    std::string curr_filename_str = file_path.string();
    curr_filename = file_path.string().c_str();

    int curr_filename_len = curr_filename_str.length();
    if (curr_filename_len > 0)
    {
        char lastChar = curr_filename_str[curr_filename_len-1];
        if (lastChar == '/' || lastChar == '\\')
        {
            LogPrintf(" extracting: creating dir %s\n", curr_filename);
            MKDIR(curr_filename);
            return UNZ_OK;
        }
    }

    buf = (void*)malloc(size_buf);
    if (buf == NULL)
    {
        LogPrintf("Error allocating memory\n");
        return UNZ_INTERNALERROR;
    }

    err = unzOpenCurrentFile(uf);
    if (err != UNZ_OK)
        LogPrintf("error %d with zipfile in unzOpenCurrentFilePassword\n", err);

    /* Create the file on disk so we can unzip to it */
    if (err == UNZ_OK)
    {
        fout = fopen64(curr_filename, "wb");
        /* Some zips don't contain directory alone before file */
        if (fout == NULL)
            LogPrintf("error opening %s\n", curr_filename);
    }

    /* Read from the zip, unzip to buffer, and write to disk */
    if (fout != NULL)
    {
        LogPrintf(" extracting: %s\n", curr_filename);

        do
        {
            err = unzReadCurrentFile(uf, buf, size_buf);
            if (err < 0)
            {
                LogPrintf("error %d with zipfile in unzReadCurrentFile\n", err);
                break;
            }
            if (err == 0)
                break;
            if (fwrite(buf, err, 1, fout) != 1)
            {
                LogPrintf("error %d in writing extracted file\n", errno);
                err = UNZ_ERRNO;
                break;
            }
        }
        while (err > 0);

        if (fout)
            fclose(fout);
    }

    errclose = unzCloseCurrentFile(uf);
    if (errclose != UNZ_OK)
        LogPrintf("error %d with zipfile in unzCloseCurrentFile\n", errclose);

    free(buf);
    return err;
}

int zip_extract_all(unzFile uf, const fs::path& root_file_path, const char * allowed_dir)
{
    int err = unzGoToFirstFile(uf);
    if (err != UNZ_OK)
    {
        LogPrintf("error %d with zipfile in unzGoToFirstFile\n", err);
        return 1;
    }

    do
    {
        err = zip_extract_currentfile(uf, root_file_path, allowed_dir);
        if (err != UNZ_OK)
            break;
        err = unzGoToNextFile(uf);
    }
    while (err == UNZ_OK);

    if (err != UNZ_END_OF_LIST_OF_FILE)
    {
        LogPrintf("error %d with zipfile in unzGoToNextFile\n", err);
        return 1;
    }
    return UNZ_OK;
}