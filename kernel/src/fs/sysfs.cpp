//=======================================================================
// Copyright Baptiste Wicht 2013-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#include <types.hpp>
#include <unique_ptr.hpp>
#include <algorithms.hpp>
#include <errors.hpp>

#include "fs/sysfs.hpp"

#include "console.hpp"
#include "rtc.hpp"

namespace {

struct sys_value {
    std::string name;
    std::string value;

    sys_value(){}
    sys_value(std::string name, std::string value) : name(name), value(value){
        //Nothing else to init
    }
};

struct sys_folder {
    std::string name;
    std::vector<sys_folder> folders;
    std::vector<sys_value> values;

    sys_folder(){}

    sys_folder(std::string name) : name(name) {
        //Nothing else to init
    }
};

std::vector<sys_folder> root_folders;

sys_folder& find_root_folder(const std::string& mount_point){
    for(auto& sys_folder : root_folders){
        if(sys_folder.name == mount_point){
            return sys_folder;
        }
    }

    root_folders.emplace_back(mount_point);

    return root_folders.back();
}

sys_folder& find_folder(sys_folder& root, const std::vector<std::string>& path, size_t i, size_t last){
    auto& name = path[i];

    for(auto& folder : root.folders){
        if(folder.name == name){
            if(i == last - 1){
                return folder;
            } else {
                return find_folder(folder, path, i + 1, last);
            }
        }
    }

    root.folders.emplace_back(name);

    if(i == last - 1){
        return root.folders.back();
    } else {
        return find_folder(root.folders.back(), path, i + 1, last);
    }
}

bool exists_folder(sys_folder& root, const std::vector<std::string>& path, size_t i, size_t last){
    auto& name = path[i];

    for(auto& folder : root.folders){
        if(folder.name == name){
            if(i == last - 1){
                return true;
            } else {
                return exists_folder(folder, path, i + 1, last);
            }
        }
    }

    return false;
}

} //end of anonymous namespace

sysfs::sysfs_file_system::sysfs_file_system(std::string mp) : mount_point(mp) {
    //Nothing to init
}

sysfs::sysfs_file_system::~sysfs_file_system(){
    //Nothing to delete
}

size_t sysfs::sysfs_file_system::get_file(const std::vector<std::string>& file_path, vfs::file& f){
    auto& root_folder = find_root_folder(mount_point);

    if(exists_folder(root_folder, file_path, 0, file_path.size() - 1)){
        auto& folder = find_folder(root_folder, file_path, 0, file_path.size() - 1);

        for(auto& file : folder.folders){
            if(file.name == file_path.back()){
                f.file_name = file.name;
                f.directory = true;
                f.hidden = false;
                f.system = false;
                f.size = 0;

                return 0;
            }
        }

        for(auto& file : folder.values){
            if(file.name == file_path.back()){
                f.file_name = file.name;
                f.directory = false;
                f.hidden = false;
                f.system = false;
                f.size = 0;

                return 0;
            }
        }
    }

    return std::ERROR_NOT_EXISTS;
}

size_t sysfs::sysfs_file_system::read(const std::vector<std::string>& file_path, std::string& content){
    auto& root_folder = find_root_folder(mount_point);

    if(exists_folder(root_folder, file_path, 0, file_path.size() - 1)){
        auto& folder = find_folder(root_folder, file_path, 0, file_path.size() - 1);

        for(auto& file : folder.values){
            if(file.name == file_path.back()){
                content = file.value;
                return 0;
            }
        }

        for(auto& file : folder.folders){
            if(file.name == file_path.back()){
                return std::ERROR_DIRECTORY;
            }
        }
    }

    return std::ERROR_NOT_EXISTS;
}

size_t sysfs::sysfs_file_system::ls(const std::vector<std::string>& file_path, std::vector<vfs::file>& contents){
    auto& root_folder = find_root_folder(mount_point);

    if(exists_folder(root_folder, file_path, 0, file_path.size() - 1)){
        auto& folder = find_folder(root_folder, file_path, 0, file_path.size() - 1);

        for(auto& file : folder.folders){
            vfs::file f;
            f.file_name = file.name;
            f.directory = true;
            f.hidden = false;
            f.system = false;
            f.size = 0;
            contents.push_back(f);
        }

        for(auto& file : folder.values){
            vfs::file f;
            f.file_name = file.name;
            f.directory = false;
            f.hidden = false;
            f.system = false;
            f.size = 0;
            contents.push_back(f);
        }

        return 0;
    } else {
        return std::ERROR_NOT_EXISTS;
    }
}

size_t sysfs::sysfs_file_system::touch(const std::vector<std::string>& ){
    return std::ERROR_PERMISSION_DENIED;
}

size_t sysfs::sysfs_file_system::mkdir(const std::vector<std::string>& ){
    return std::ERROR_PERMISSION_DENIED;
}

size_t sysfs::sysfs_file_system::rm(const std::vector<std::string>& ){
    return std::ERROR_PERMISSION_DENIED;
}

size_t sysfs::sysfs_file_system::statfs(statfs_info& file){
    file.total_size = 0;
    file.free_size = 0;

    return 0;
}

void set_value(const std::string& mount_point, const std::string& path, const std::string& value){
    auto& root_folder = find_root_folder(mount_point);

    auto file_path = std::split(path, '/');
    auto last = file_path.back();
    file_path.pop_back();

    auto& folder = find_folder(root_folder, file_path, 0, file_path.size());

    for(auto& v : folder.values){
        if(v.name == last){
            v.value = value;
            return;
        }
    }

    folder.values.emplace_back(last, value);
}

void delete_value(const std::string& mount_point, const std::string& path){
    auto& root_folder = find_root_folder(mount_point);

    auto file_path = std::split(path, '/');
    auto last = file_path.back();
    file_path.pop_back();

    auto& folder = find_folder(root_folder, file_path, 0, file_path.size());

    for(size_t i = 0; i < folder.values.size(); ++i){
        auto& v = folder.values[i];

        if(v.name == last){
            folder.values.erase(i);
            break;
        }
    }
}

void delete_folder(const std::string& mount_point, const std::string& path){
    auto& root_folder = find_root_folder(mount_point);

    auto file_path = std::split(path, '/');
    auto last = file_path.back();
    file_path.pop_back();

    auto& folder = find_folder(root_folder, file_path, 0, file_path.size());

    for(size_t i = 0; i < folder.folders.size(); ++i){
        auto& v = folder.folders[i];

        if(v.name == last){
            folder.folders.erase(i);
            break;
        }
    }
}
