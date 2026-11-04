#include "webserver.h"

#include "dimensionsendpoints.h"
#include "files.h"
#include "status.h"

#include "http.hpp"
#include "utils/logger.h"

#include <coreinit/thread.h>
#include <nn/ac/ac_c.h>

#include <mutex>
#include <chrono>
#include <cstdio>
#include <exception>
#include <thread>

// The page itself, embedded from data/toypad.html by the Makefile's bin2o rule.
// bin2s always emits the start and end labels; the size is their difference.
extern "C" const uint8_t toypad_html[];
extern "C" const uint8_t toypad_html_end[];

// One server per running application: Wii U threads do not outlive the
// application that created them, so the server (and the cleanup thread its
// constructor starts) is created when a game starts and dropped when it ends.
static std::mutex sServerMutex;
static HttpServer *sServer = nullptr;

static void registerRoutes(HttpServer &server) {
    // The control page. Open http://<console IP>:8853 in any browser on the same network.
    server.when("/")->requested([](const HttpRequest &req) {
        HttpResponse res{200, "text/html; charset=utf-8",
                         std::string(reinterpret_cast<const char *>(toypad_html), size_t(toypad_html_end - toypad_html))};
        res["Cache-Control"] = "no-cache";
        return res;
    });

    // Plain-text probe, kept for tools that check the server is up.
    server.when("/ping")->requested([](const HttpRequest &req) {
        return HttpResponse{200, "text/plain", "re_nsyshid"};
    });

    registerStatusEndpoints(server);
    registerDimensionsEndpoints(server);
    registerFileEndpoints(server);
}

static void serverThread(HttpServer *server) {
    try {
        DEBUG_FUNCTION_LINE_INFO("Web server listening on port %d", RE_NSYSHID_WEB_PORT);
        // Blocks until stopWebServer() closes the listening socket.
        server->startListening(RE_NSYSHID_WEB_PORT);
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_ERR("Web server stopped with an exception: %s", e.what());
    }

    {
        std::lock_guard lock(sServerMutex);
        if (sServer == server) {
            sServer = nullptr;
        }
    }
    // Connection threads were told to close by shutdown(); give any request
    // still being answered time to finish before the server goes away.
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // Joins the server's cleanup thread.
    delete server;
}

void startWebServer() {
    std::lock_guard lock(sServerMutex);
    if (sServer) {
        return;
    }
    try {
        auto *server = new HttpServer();
        registerRoutes(*server);

        std::thread thread(serverThread, server);
        auto handle = (OSThread *) thread.native_handle();
        OSSetThreadName(handle, "re_nsyshid_http_thread");
        // Keep it off the cores the game leans on hardest.
        OSSetThreadAffinity(handle, OS_THREAD_ATTRIB_AFFINITY_CPU2);
        thread.detach();
        sServer = server;
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_ERR("Could not start the web server: %s", e.what());
    }
}

void stopWebServer() {
    std::lock_guard lock(sServerMutex);
    if (!sServer) {
        return;
    }
    // The server thread wakes from accept(), returns and deletes the server.
    sServer->shutdown();
    sServer = nullptr;
    DEBUG_FUNCTION_LINE_INFO("Web server stopped");
}

bool isWebServerRunning() {
    std::lock_guard lock(sServerMutex);
    return sServer != nullptr;
}

std::string webServerAddress() {
    uint32_t ip = 0;
    if (NNResult_IsFailure(ACInitialize())) {
        return "";
    }
    const bool ok = NNResult_IsSuccess(ACGetAssignedAddress(&ip)) && ip != 0;
    ACFinalize();
    if (!ok) {
        return "";
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "http://%u.%u.%u.%u:%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
             (ip >> 8) & 0xFF, ip & 0xFF, RE_NSYSHID_WEB_PORT);
    return buf;
}
