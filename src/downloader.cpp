#include <downloader.h>

#include <init.h>
#include <logging.h>
#include <util/system.h>

#include <util/miniunz.h>
#include <curl/curl.h>
#include <openssl/ssl.h>

/*  Downloader functions for bootstrapping and updating client software

 * This xferinfo_data contains a callback function to be called
 * if the value is not nullptr.
 *
 * void xferinfo_data(curl_off_t total, curl_off_t now);
 *
 * This is admittedly ugly, but it allows us to get a percentage
 * callback in the GUI portion of code. by setting the xinfo_data.
 *
 * XXX it could use some rate limiting
 */
static void* xferinfo_data = nullptr;
static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
    void (*ptr)(curl_off_t, curl_off_t) = (void(*)(curl_off_t, curl_off_t))xferinfo_data;
    if (ptr != nullptr) ptr(dltotal, dlnow);
    return 0; // continue xfer.
}

void set_xferinfo_data(void* d)
{
    xferinfo_data = d;
}

void downloadFile(std::string url, const fs::path& target_file_path) {

    LogPrintf("Download: Downloading from %s. \n", url);

    FILE *file = fsbridge::fopen(target_file_path, "wb");
    if( ! file )
        throw std::runtime_error(strprintf("Download: error: Unable to open output file for writing: %s.", target_file_path.c_str()));

    CURL *curlHandle = curl_easy_init();

    CURLcode res;
    char errbuf[CURL_ERROR_SIZE];

    curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errbuf);
    errbuf[0] = 0;

    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curlHandle, CURLOPT_XFERINFODATA, xferinfo_data);
    curl_easy_setopt(curlHandle, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, file);
    res = curl_easy_perform(curlHandle);

    if(res != CURLE_OK) {
        curl_easy_cleanup(curlHandle);
        size_t len = strlen(errbuf);
        if(len)
            throw std::runtime_error(strprintf("Download: error: %s%s.", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : "")));
        else
            throw std::runtime_error(strprintf("Download: error: %s.", curl_easy_strerror(res)));
    }

    long response_code;
    curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &response_code);
    if( response_code != 200 )
        throw std::runtime_error(strprintf("Download: error: Server responded with a %d .", response_code));

    curl_easy_cleanup(curlHandle);
    fclose(file);

    LogPrintf("Download: Successful.\n");

    return;
}


// bootstrap
void extractBootstrap(const fs::path& target_file_path) {
    LogPrintf("bootstrap: Extracting bootstrap %s.\n", target_file_path);

    if (!boost::filesystem::exists(target_file_path))
        throw std::runtime_error("bootstrap: Bootstrap archive not found");


    const char * zipfilename = target_file_path.string().c_str();
    unzFile uf;
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def ffunc;
    fill_win32_filefunc64A(&ffunc);
    uf = unzOpen2_64(zipfilename, &ffunc);
#else
    uf = unzOpen64(zipfilename);
#endif

    if (uf == NULL)
        throw std::runtime_error(strprintf("bootstrap: Cannot open bootstrap archive: %s\n", zipfilename));

    int unzip_err = zip_extract_all(uf, GetDataDir(), "bootstrap");
    if (unzip_err != UNZ_OK)
        throw std::runtime_error("bootstrap: Unzip failed\n");

    LogPrintf("bootstrap: Unzip successful\n");

    return;
}

void validateBootstrapContent() {


    LogPrintf("bootstrap: Checking Bootstrap Content\n");

    if (!boost::filesystem::exists(GetDataDir() / "bootstrap" / "chainstate") ||
        !boost::filesystem::exists(GetDataDir() / "bootstrap" / "blocks"))
        throw std::runtime_error("bootstrap: Downloaded zip file did not contain all necessary files!\n");

}

void applyBootstrap() {
    boost::filesystem::remove_all(GetDataDir() / "blocks");
    boost::filesystem::remove_all(GetDataDir() / "chainstate");
    boost::filesystem::rename(GetDataDir() / "bootstrap" / "blocks", GetDataDir() / "blocks");
    boost::filesystem::rename(GetDataDir() / "bootstrap" / "chainstate", GetDataDir() / "chainstate");
    boost::filesystem::remove_all(GetDataDir() / "bootstrap");
    boost::filesystem::path pathBootstrapTurbo(GetDataDir() / "bootstrap_VRM.zip");
    boost::filesystem::path pathBootstrap(GetDataDir() / "bootstrap.dat");
    if (boost::filesystem::exists(pathBootstrapTurbo)){
        boost::filesystem::remove(pathBootstrapTurbo);
    }
    if (boost::filesystem::exists(pathBootstrap)){
        boost::filesystem::remove(pathBootstrap);
	}
}

void downloadBootstrap() {
    LogPrintf("bootstrap: Starting bootstrap process.\n");

    boost::filesystem::path pathBootstrapZip = GetDataDir() / "bootstrap_VRM.zip";

    downloadFile(BOOTSTRAP_URL, pathBootstrapZip);
    extractBootstrap(pathBootstrapZip);
    validateBootstrapContent();

    fBootstrap = true;

    LogPrintf("bootstrap: bootstrap process finished.\n");

    return;
}

// check for update
void downloadVersionFile() {
    LogPrintf("Check for update: Getting version file.\n");

    boost::filesystem::path pathVersionFile = GetDataDir() / "VERSION_VRM.json";

    downloadFile(VERSIONFILE_URL, pathVersionFile);

    return;
}

void downloadClient(std::string fileName) {
    LogPrintf("Check for update: Downloading new client.\n");

    boost::filesystem::path pathClientFile = GetDataDir() / fileName;
    std::string clientFileUrl = CLIENT_URL + fileName;

    downloadFile(clientFileUrl, pathClientFile);

    return;
}

int getArchitecture()
{
    int *i;
    return sizeof(i) * 8; // 8 bits/byte
}