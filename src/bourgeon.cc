#include "bourgeon.h"

#include <Windows.h>

#define VERSION_MAJOR "0"
#define VERSION_MINOR "1"

Bourgeon::Bourgeon()
    : console_(), interpreter_(), callbacks_(), plugin_threads_(), client_() {}

RagnarokClient& Bourgeon::client() { return client_; }

bool Bourgeon::Initialize() {
  console_.LogInfo("Bourgeon " VERSION_MAJOR "." VERSION_MINOR "\n");

  if (!client_.Initialize()) {
    console_.LogError(
        "Bourgeon failed to initialize, your client is most likely not "
        "supported. (yet!)");
    return false;
  }
  console_.LogInfo("Bourgeon initialized successfully !");
  console_.LogInfo("Detected client: " + std::to_string(client_.timestamp()));
  LoadPlugins("./plugins");

  return true;
}

void Bourgeon::RunTick() {
  // Process ticks indefinitely
  for (;;) {
    for (auto& registree : callbacks_["OnTick"]) {
      try {
        registree();
      } catch (pybind11::error_already_set& error) {
        console_.LogError(error.what());
        UnregisterCallback("OnTick", registree);
      }
    }
    Sleep(100);
  }
}

void Bourgeon::RegisterCallback(const std::string& callback_name,
                                const pybind11::object& function) {
  try {
    console_.LogInfo(function.attr("__name__").cast<std::string>() +
                     " has been registered to " + callback_name);
  } catch (pybind11::error_already_set& error) {
    console_.LogError(error.what());
  }
  callbacks_[callback_name].push_back(function);
}

void Bourgeon::UnregisterCallback(const std::string& callback_name,
                                  const pybind11::object& function) {
  for (auto it = callbacks_[callback_name].begin();
       it != callbacks_[callback_name].end(); ++it) {
    if (function.ptr() == it->ptr()) {
      callbacks_[callback_name].erase(it);
      console_.LogInfo(function.attr("__name__").cast<std::string>() +
                       " has been unregistered from " + callback_name);
      break;
    }
  }
}

const std::vector<pybind11::object>& Bourgeon::GetCallbackRegistrees(
    const std::string& callback_name) {
  return callbacks_[callback_name];
}

void Bourgeon::LoadPlugins(const std::string& folder) {
  using namespace pybind11;

  std::string search_path = folder + "/*.py";
  WIN32_FIND_DATA fd;
  HANDLE h_find = FindFirstFileA(search_path.c_str(), &fd);

  if (h_find == INVALID_HANDLE_VALUE) return;

  do {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

    std::string filename(fd.cFileName);
    console_.LogInfo("Loading " + filename);
    try {
      eval_file(folder + '/' + filename,
                module::import("__main__").attr("__dict__"));
    } catch (error_already_set& error) {
      console_.LogError(error.what());
    }
  } while (FindNextFileA(h_find, &fd));

  FindClose(h_find);
}