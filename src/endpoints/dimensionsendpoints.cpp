#include "dimensionsendpoints.h"
#include "../devices/Dimensions.h"
#include "files.h"

#include <chrono>
#include <mutex>
#include <thread>

// How long a figure stays "lifted" between leaving one space and landing on
// another. The game needs to see the removal before the arrival, the same as
// when the move is done from the in-game menu (A to pick up, A to put down).
static constexpr auto kMovePickupDelay = std::chrono::milliseconds(500);

// Web requests arrive on their own threads. Run toypad changes one at a time
// so a load can't land in the middle of a move.
static std::mutex sToypadCommandMutex;

static uint8_t derivePadFromIndex(uint8_t index) {
    switch (index) {
        case 0:
        case 3:
        case 4:
            return 2;

        case 2:
        case 5:
        case 6:
            return 3;

        case 1:
            return 1;

        default:
            return 0;
    }
}

static HttpResponse corsPreflight(const HttpRequest &) {
    HttpResponse res(200);
    res["Access-Control-Allow-Methods"] = "POST, OPTIONS";
    res["Access-Control-Allow-Headers"] = "Content-Type";
    res["Access-Control-Max-Age"]       = "86400";
    return res;
}

// Reads a slot number (0-6) out of a JSON body field.
static bool readSlot(miniJson::Json::_object &body, const char *key, uint8_t &slotOut) {
    const auto value = body[key];
    if (!value.isNumber()) {
        return false;
    }
    const double slot = value.toDouble();
    if (slot < 0 || slot >= 7) {
        return false;
    }
    slotOut = uint8_t(slot);
    return true;
}

static miniJson::Json::_object currentFiguresJson() {
    miniJson::Json::_object ret;
    std::array<std::optional<uint32_t>, 7> currentFigures = g_dimensionstoypad.GetCurrentFigures();
    for (uint8_t i = 0; i < 7; i++) {
        miniJson::Json::_object figure;
        if (currentFigures[i]) {
            figure["id"]   = int(currentFigures[i].value());
            figure["name"] = g_dimensionstoypad.FindFigure(currentFigures[i].value());
        } else {
            figure["id"]   = 0;
            figure["name"] = "None";
        }
        ret[std::to_string(i)] = figure;
    }
    return ret;
}

static miniJson::Json::_object padColorsJson() {
    miniJson::Json::_object res;
    auto colors = g_dimensionstoypad.GetPadColors();
    for (uint8_t i = 0; i < 3; i++) {
        miniJson::Json::_object color;
        color["red"]           = int(colors[i].red);
        color["green"]         = int(colors[i].green);
        color["blue"]          = int(colors[i].blue);
        color["speed"]         = int(colors[i].speed);
        color["cycles"]        = int(colors[i].cycles);
        color["whiteDuration"] = int(colors[i].whiteDuration);
        color["colorDuration"] = int(colors[i].colorDuration);
        switch (colors[i].colorType) {
            case DimensionsToypad::DimensionsColorType::NONE:
                color["type"] = "none";
                break;
            case DimensionsToypad::DimensionsColorType::COLOR:
                color["type"] = "color";
                break;
            case DimensionsToypad::DimensionsColorType::FADE:
                color["type"] = "fade";
                break;
            case DimensionsToypad::DimensionsColorType::FLASH:
                color["type"] = "flash";
                break;
        }
        static const char *padNames[3] = {"top", "left", "right"};
        res[padNames[i]]               = color;
    }
    return res;
}

void registerDimensionsEndpoints(HttpServer &server) {

    server.when("/device/dimensions")->requested([](const HttpRequest &req) {
        return HttpResponse{200, currentFiguresJson()};
    });

    // Slots and pad lights in one request, so the web page can poll once.
    server.when("/device/dimensions/state")->requested([](const HttpRequest &req) {
        miniJson::Json::_object ret;
        ret["slots"]  = currentFiguresJson();
        ret["colors"] = padColorsJson();
        return HttpResponse{200, ret};
    });

    server.when("/device/dimensions/colors")->requested([](const HttpRequest &req) {
        return HttpResponse{200, padColorsJson()};
    });

    server.when("/device/dimensions/remove")
            ->options(corsPreflight)
            ->posted([](const HttpRequest &req) {
                miniJson::Json::_object res;
                auto body = req.json();
                if (!body.isObject()) {
                    res["error"] = "INVALID_BODY";
                    return HttpResponse{400, res};
                }
                auto removeRequest = body.toObject();
                uint8_t slot;
                if (!readSlot(removeRequest, "slot", slot)) {
                    res["error"] = "INVALID_SLOT";
                    return HttpResponse{400, res};
                }

                std::lock_guard lock(sToypadCommandMutex);
                if (g_dimensionstoypad.RemoveFigure(derivePadFromIndex(slot), slot, true)) {
                    res["message"] = "Figure removed";
                    return HttpResponse{200, res};
                }
                res["error"] = "NO_FIGURE_IN_SLOT";
                return HttpResponse{404, res};
            });

    server.when("/device/dimensions/load")
            ->options(corsPreflight)
            ->posted([](const HttpRequest &req) {
                miniJson::Json::_object res;
                auto body = req.json();
                if (!body.isObject()) {
                    res["error"] = "INVALID_BODY";
                    return HttpResponse{400, res};
                }
                auto loadRequest = body.toObject();
                uint8_t slot;
                if (!readSlot(loadRequest, "slot", slot)) {
                    res["error"] = "INVALID_SLOT";
                    return HttpResponse{400, res};
                }
                const auto fileParam = loadRequest["file"];
                if (!fileParam.isString()) {
                    res["error"] = "MISSING_FILE_PARAM";
                    return HttpResponse{400, res};
                }
                const std::string relative = fileParam.toString();
                if (relative.empty() || !isSafeRelativePath(relative)) {
                    res["error"] = "INVALID_FILE";
                    return HttpResponse{400, res};
                }
                const std::string file = std::string(RE_NSYSHID_FIGURE_ROOT) + relative;

                std::array<uint8_t, DIM_FIGURE_SIZE> buf;
                int result = FSUtils::ReadFromFile(file.c_str(), buf.data(), buf.size());
                if (result != DIM_FIGURE_SIZE) {
                    res["error"] = "COULD_NOT_READ_FILE";
                    return HttpResponse{400, res};
                }

                std::lock_guard lock(sToypadCommandMutex);
                const uint8_t pad = derivePadFromIndex(slot);
                // Placing on an occupied space swaps the figure out, the
                // same as lifting the old one off first.
                g_dimensionstoypad.RemoveFigure(pad, slot, true);
                uint32_t id = g_dimensionstoypad.LoadFigure(buf, file, pad, slot);

                res["message"]   = "Figure loaded";
                res["figure_id"] = int(id);
                res["name"]      = g_dimensionstoypad.FindFigure(id);
                return HttpResponse{200, res};
            });

    server.when("/device/dimensions/move")
            ->options(corsPreflight)
            ->posted([](const HttpRequest &req) {
                miniJson::Json::_object res;
                auto body = req.json();
                if (!body.isObject()) {
                    res["error"] = "INVALID_BODY";
                    return HttpResponse{400, res};
                }
                auto moveRequest = body.toObject();
                uint8_t oldSlot, newSlot;
                if (!readSlot(moveRequest, "oldSlot", oldSlot) || !readSlot(moveRequest, "newSlot", newSlot)) {
                    res["error"] = "INVALID_SLOT";
                    return HttpResponse{400, res};
                }
                if (oldSlot == newSlot) {
                    res["message"] = "Figure not moved";
                    return HttpResponse{200, res};
                }

                std::lock_guard lock(sToypadCommandMutex);
                // Lift the figure, give the game a moment to notice, then put
                // it down on the new space (which removes anything already
                // there).
                if (!g_dimensionstoypad.TempRemove(oldSlot)) {
                    res["error"] = "NO_FIGURE_IN_OLD_SLOT";
                    return HttpResponse{404, res};
                }
                std::this_thread::sleep_for(kMovePickupDelay);
                g_dimensionstoypad.MoveFigure(derivePadFromIndex(newSlot), newSlot,
                                              derivePadFromIndex(oldSlot), oldSlot);
                res["message"] = "Figure moved";
                return HttpResponse{200, res};
            });
}
