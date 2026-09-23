/*
    Copyright (C) 2026 Glacc

    This file is part of pac-dep-graph.

    pac-dep-graph is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    pac-dep-graph is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with pac-dep-graph. If not,
    see <https://www.gnu.org/licenses/>. 
*/

#include <cstdio>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <fstream>

struct PkgInfo
{
    std::string version;
    std::vector<std::string> parent;
    std::vector<std::string> child;
};

static const char *indicator_symbols[] =
{
    "├─",
    "│ ",
    "└─",
    "  ",
};

static void RecursiveListPkgRequirementTree(std::map<std::string, PkgInfo> &packages, const std::string &pkg_name, PkgInfo &pkg_info, std::vector<int> &indicators, std::set<std::string> &visited, int depth)
{
    // std::cout << (!depth ? "─" : " ");
    for (int indicator_no : indicators)
        std::cout << indicator_symbols[indicator_no];
    std::cout << pkg_name;

    bool has_unvisited_child = false;
    for (std::string child_pkg_name : pkg_info.child)
    {
        if (!visited.contains(child_pkg_name))
        {
            has_unvisited_child = true;
            break;
        }
    }

    if (has_unvisited_child)
    {
        std::cout << " ↓\n";
        return;
    }

    if (visited.contains(pkg_name))
    {
        std::cout << " ↑\n";
        return;
    }

    visited.insert(pkg_name);

    std::cout << '\n';

    int depth_next = depth + 1;

    if (indicators.size() >= 1)
    {
        int indicator_last_index = indicators.size() - 1;

        if (indicators[indicator_last_index] == 0)
            indicators[indicator_last_index] = 1;

        if (indicators[indicator_last_index] == 2)
            indicators[indicator_last_index] = 3;
    }

    std::vector<std::string> &parent_pkgs = pkg_info.parent;
    int parent_dep_count = parent_pkgs.size();

    for (int i = 0; i < parent_dep_count; i++)
    {
        const std::string &parent_pkg_name = parent_pkgs[i];

        bool is_last = true;

        for (int j = i + 1; j < parent_dep_count; j++)
        {
            std::string next_parent_pkg_name = parent_pkgs[j];

            if (!packages.contains(next_parent_pkg_name))
                continue;

            for (const std::string &child_pkg_name_of_next_parent : packages[next_parent_pkg_name].child)
            {
                if (visited.contains(child_pkg_name_of_next_parent))
                {
                    is_last = false;
                    goto end_check;
                }
            }
        }
        end_check:

        indicators.push_back(is_last ? 2 : 0);

        if (packages.contains(parent_pkg_name))
            RecursiveListPkgRequirementTree(packages, parent_pkg_name, packages[parent_pkg_name], indicators, visited, depth_next);

        indicators.pop_back();
    }
}

void ReverseListPackagesWithoutChild(std::map<std::string, PkgInfo> &packages)
{
    std::set<std::string> visited;
    std::vector<int> indicators;

    for (auto &package : packages)
    {
        if (package.second.child.empty())
        {
            visited.clear();

            RecursiveListPkgRequirementTree(packages, package.first, package.second, indicators, visited, 0);

            std::cout << '\n';
        }
    }
}

int main(int argc, char **argv)
{
    std::map<std::string, PkgInfo> packages;

    // read
    std::stringstream ss_pkg_info;
    std::FILE *pipe = popen("pacman -Qi", "r");
    if (pipe)
    {
        char buffer[512];

        while (std::fgets(buffer, sizeof(buffer), pipe))
            ss_pkg_info << buffer;

        pclose(pipe);
    }

    // get
    std::string line;
    int read_mode = 0;
    const int read_mode_depends_on = 1;
    const int read_mode_required_by = 2;

    std::string pkg_name;
    
    while (std::getline(ss_pkg_info, line))
    {
        int read_index = 0;

        int offset_colon = line.find(':');

        if (line.starts_with("Name"))
        {
            read_mode = 0;
            pkg_name = line.substr(offset_colon + 2);

            if (!packages.contains(pkg_name))
                packages.insert(std::pair(pkg_name, PkgInfo()));
        }

        if (line.starts_with("Version"))
        {
            if (packages.contains(pkg_name))
                packages[pkg_name].version = line.substr(offset_colon + 1);
        }

        if (line.starts_with("Depends On"))
        {
            read_mode = read_mode_depends_on;
            read_index = offset_colon + 2;
        }

        if (line.starts_with("Optional Deps"))
            read_mode = 0;

        if (line.starts_with("Required By"))
        {
            read_mode = read_mode_required_by;
            read_index = offset_colon + 2;
        }

        if (line.starts_with("Optional For"))
            read_mode = 0;

        if (!read_mode)
            continue;

        char char_last = '\0';

        std::vector<char> vec_curr_dep_name;

        while (read_index <= line.size())
        {
            char char_curr = line[read_index];

            if ((char_last != ' ' && char_curr == ' ') || char_curr == '\0' || char_curr == '>' || char_curr == '=')
            {
                if (vec_curr_dep_name.size() > 0)
                {
                    vec_curr_dep_name.push_back('\0');

                    if (strcmp(vec_curr_dep_name.data(), "None"))
                    {
                        if (read_mode == read_mode_depends_on)
                            packages[pkg_name].parent.push_back(std::string(vec_curr_dep_name.data()));

                        if (read_mode == read_mode_required_by)
                            packages[pkg_name].child.push_back(std::string(vec_curr_dep_name.data()));
                    }
                }

                vec_curr_dep_name.clear();
            }

            if (char_curr == '\0')
                break;

            if (char_curr != ' ' && char_curr != '\0' && char_curr != '=' && char_curr != '>')
                vec_curr_dep_name.push_back(char_curr);

            char_last = char_curr;

            read_index++;
        }
    }

    // list
    ReverseListPackagesWithoutChild(packages);

    // draw
    std::fstream graphviz_file("./pac-dep-graph.gv", std::fstream::out | std::fstream::trunc);

    if (graphviz_file.is_open())
    {
        graphviz_file <<
R"rs(digraph {
    label="ArchLinuxDependencyGraph"

    layout=sfdp
    beautify=true
    overlap_scaling=8
    K=4

    node [fontsize=10,margin="0.0,0.0",height=0.25,shape=box]
    edge [color="#00000040",arrowsize=0.4]

)rs";

        for (const auto &package : packages)
        {
            graphviz_file << "    \"" << package.first << '\"';

            if (!package.second.child.empty())
            {
                graphviz_file << " -> ";
                if (package.second.child.size() > 1) graphviz_file << '{';

                bool first = true;
                for (const std::string &dep_name : package.second.child)
                {
                    if (!first) graphviz_file << ",";
                    graphviz_file << "\"" << dep_name << "\"";
                    first = false;
                }

                if (package.second.child.size() > 1) graphviz_file << '}';
            }
            else graphviz_file << " [color=\"#FF0000FF\",fontcolor=\"#FF0000FF\"]";
            
            graphviz_file << ";\n";
        }

        graphviz_file << "}\n";

        graphviz_file.close();
    }

    return 0;
}
