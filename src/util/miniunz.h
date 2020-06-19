#ifndef BITCOIN_UTIL_MINIUNZ_H
#define BITCOIN_UTIL_MINIUNZ_H

#include <boost/filesystem.hpp>
#include <minizip/unzip.h>
#include <fs.h>

int zip_extract_all(unzFile uf, const fs::path& root_file_path, const char * allowed_dir);

#endif // BITCOIN_UTIL_MINIUNZ_H
