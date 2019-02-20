// Copyright (c) 2017 The Verium developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MINIZIP_MINIUNZ_H
#define MINIZIP_MINIUNZ_H

#include <boost/filesystem.hpp>
#include <minizip/unzip.h>

int zip_extract_all(unzFile uf, boost::filesystem::path root_file_path, const char * allowed_dir);

#endif // MINIZIP_MINIUNZ_H

