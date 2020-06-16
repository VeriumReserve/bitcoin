#ifndef BITCOIN_DOWNLOADER_H
#define BITCOIN_DOWNLOADER_H

#include <string>

const std::string BOOTSTRAP_URL("https://cdn.vericonomy.com/016-bootstrap/bootstrap_VRM.zip");
const std::string VERSIONFILE_URL("https://cdn.vericonomy.com/updates/VERSION_VRM.json");
const std::string CLIENT_URL("https://cdn.vericonomy.com/verium/");

void downloadBootstrap();
void applyBootstrap();
void downloadVersionFile();
void downloadClient(std::string fileName);
int getArchitecture();

#endif // BITCOIN_DOWNLOADER_H
