#ifndef _SNF_PLUGIN_H_
#define _SNF_PLUGIN_H_

#include <memory>
#include <vector>
#include "dll.h"

namespace snf {

class plugin
{
private:
    dll *m_dll = nullptr;

    plugin() {}
    void attach_one(const std::string &);

public:
    ~plugin() { if (m_dll) delete m_dll; }

    static std::unique_ptr<plugin> attach(const std::string &);
    static std::vector<std::unique_ptr<plugin>> attach_all(const std::string &);
    void detach();

    template<typename T>
    T *import(const std::string &name, bool fatal = false)
    {
        return m_dll ? reinterpret_cast<T *>(m_dll->symbol(name.c_str(), fatal)) : nullptr;
    }
};

} // namespace snf

#endif // _SNF_PLUGIN_H_