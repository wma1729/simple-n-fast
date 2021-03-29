#include "dir.h"
#include "fattr.h"
#include "filesystem.h"
#include "plugin.h"

#if defined(_WIN32)
#define PLUGIN_PATTERN  R"(*.dll)"
#else
#define PLUGIN_PATTERN  R"(*.so)"
#endif

namespace snf {

void
plugin::attach_one(const std::string &path)
{
    if (snf::fs::exists(path.c_str())) {
        m_dll = new dll(path, true);
    } else {
        std::ostringstream oss;
        oss << "path " << path << " does not exist";
        throw std::invalid_argument(oss.str());
    }
}

std::unique_ptr<plugin>
plugin::attach(const std::string &path)
{
    plugin *p = DBG_NEW plugin();
    p->attach_one(path);
    return std::unique_ptr<plugin>{p};
}

std::vector<std::unique_ptr<plugin>>
plugin::attach_all(const std::string &path)
{
    file_attr fa_path(path);
    if (fa_path.f_type == file_type::directory) {
        std::vector<std::unique_ptr<plugin>> plugins;

        for (auto &ent : directory(path, PLUGIN_PATTERN)) {
            std::ostringstream oss;
            oss << path << snf::pathsep() << ent.f_name;
            plugins.emplace_back(attach(oss.str()));
        }

        return plugins;
    } else {
        std::ostringstream oss;
        oss << "path " << path << " is of type " << file_type_string(fa_path.f_type);
        throw std::invalid_argument(oss.str());
    }
}

void
plugin::detach()
{
    if (m_dll) {
        delete m_dll;
        m_dll = nullptr;
    }
}

} // namespace snf