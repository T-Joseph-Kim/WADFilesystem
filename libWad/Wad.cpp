#include "Wad.h"
#include <cstring>
#include <iostream>
#include <algorithm>

Wad::Wad() : root(new Node(true, nullptr, "/")) {}

Wad::~Wad() {
    delete root;
    for (auto lump : lumps) {
        delete lump;
    }
}

Wad* Wad::loadWad(const std::string &path) {
    Wad *wad = new Wad();
    wad->wadPath = path;
    wad->parseWadFile(path);
    wad->buildDirectoryStructure();
    return wad;
}

void Wad::parseWadFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open WAD file.");
    }

    char magicBuffer[5];
    file.read(magicBuffer, 4);
    magicBuffer[4] = '\0';
    magic = std::string(magicBuffer);

    file.read(reinterpret_cast<char*>(&descriptorCount), sizeof(int));

    file.read(reinterpret_cast<char*>(&descriptorOffset), sizeof(int));

    file.seekg(descriptorOffset, std::ios::beg);
    for (int i = 0; i < descriptorCount; ++i) {
        Lump *lump = new Lump();
        file.read(reinterpret_cast<char*>(&lump->offset), sizeof(int));
        file.read(reinterpret_cast<char*>(&lump->size), sizeof(int));

        char nameBuffer[9];
        file.read(nameBuffer, 8);
        nameBuffer[8] = '\0';
        lump->name = std::string(nameBuffer);
        lumps.push_back(lump);
    }
}

void Wad::buildDirectoryStructure() {
    Node *currentDir = root;
    std::vector<Node*> dirStack;
    dirStack.push_back(root);

    for (size_t i = 0; i < lumps.size(); ++i) {
        Lump *lump = lumps[i];
        std::string name = lump->name;

        if (name.size() >= 6 && name.substr(name.size() - 6) == "_START") {
            std::string dirName = name.substr(0, name.size() - 6);
            Node *newDir = new Node(true, currentDir, dirName);
            currentDir->children[dirName] = newDir;
            currentDir->orderedChildren.push_back(dirName);
            dirStack.push_back(currentDir);
            currentDir = newDir;

        } else if (name.size() >= 4 && name.substr(name.size() - 4) == "_END") {
            if (!dirStack.empty()) {
                currentDir = dirStack.back();
                dirStack.pop_back();
            }

        } else if (name.size() == 4 && name[0] == 'E' && name[2] == 'M') {
            Node *mapDir = new Node(true, currentDir, name);
            currentDir->children[name] = mapDir;
            currentDir->orderedChildren.push_back(name);

            for (int j = 1; j <= 10 && i + j < lumps.size(); ++j) {
                Lump *mapLump = lumps[i + j];
                Node *fileNode = new Node(false, mapDir, mapLump->name);
                fileNode->lump = mapLump;
                mapDir->children[mapLump->name] = fileNode;
                mapDir->orderedChildren.push_back(mapLump->name);
            }
            i += 10;
        } else {
            Node *fileNode = new Node(false, currentDir, name);
            fileNode->lump = lump;
            currentDir->children[name] = fileNode;
            currentDir->orderedChildren.push_back(name);
        }
    }
}

std::string Wad::getMagic() const {
    return magic;
}

bool Wad::isContent(const std::string &path) const {
    Node* node = findNode(path);
    bool result = node != nullptr && !node->isDirectory;
    return result;
}

bool Wad::isDirectory(const std::string &path) const {
    Node* node = findNode(path);
    bool result = node != nullptr && node->isDirectory;
    return result;
}

int Wad::getSize(const std::string &path) const {
    Node *node = findNode(path);
    if (!node) {
        return -1;
    }
    if (!node->isDirectory && node->lump) {
        return node->lump->size;
    }
    return -1;
}

int Wad::getContents(const std::string &path, char *buffer, int length, int offset) const {
    Node *node = findNode(path);
    if (!node) {
        return -1;
    }

    if (node->isDirectory) {
        return -1;
    }

    if (!node->lump) {
        return -1;
    }

    Lump *lump = node->lump;
    int size = lump->size;

    if (offset >= size) {
        return 0;
    }

    int bytesToCopy = std::min(length, size - offset);

    std::ifstream file(wadPath, std::ios::binary);
    if (!file.is_open()) {
        return -1;
    }

    file.seekg(lump->offset + offset, std::ios::beg);
    if (!file) {
        return -1;
    }

    file.read(buffer, bytesToCopy);
    if (!file) {
        return -1;
    }

    return bytesToCopy;
}

int Wad::getDirectory(const std::string &path, std::vector<std::string> *directory) const {
    Node* node = findNode(path);
    if (node && node->isDirectory) {
        for (const auto &childName : node->orderedChildren) {
            directory->push_back(childName);
        }
        return directory->size();
    }
    return -1;
}


Wad::Node* Wad::findNode(const std::string &path) const {
    if (path.empty()) return nullptr;

    std::string normalizedPath = path;
    if (normalizedPath.size() > 1 && normalizedPath.back() == '/') {
        normalizedPath.pop_back();
    }
    while (normalizedPath.find("//") != std::string::npos) {
        normalizedPath.replace(normalizedPath.find("//"), 2, "/");
    }

    Node *current = root;
    size_t start = 1, end = normalizedPath.find('/', start);

    while (end != std::string::npos) {
        std::string part = normalizedPath.substr(start, end - start);
        if (current->children.find(part) == current->children.end()) {
            return nullptr;
        }
        current = current->children[part];
        start = end + 1;
        end = normalizedPath.find('/', start);
    }

    std::string lastPart = normalizedPath.substr(start);
    if (!lastPart.empty() && current->children.find(lastPart) == current->children.end()) {
        return nullptr;
    }

    return lastPart.empty() ? current : current->children[lastPart];
}


void Wad::createDirectory(const std::string &path) {
    std::string normalizedPath = path;
    if (!normalizedPath.empty() && normalizedPath.back() == '/') {
        normalizedPath.pop_back();
    }
    std::string parentPath = normalizedPath.substr(0, normalizedPath.find_last_of('/'));
    if (parentPath.empty()) {
        parentPath = "/";
    }

    Node *parentDir = findNode(parentPath);
    if (!parentDir || !parentDir->isDirectory) {
        return;
    }

    if (parentDir->name.size() == 4 && parentDir->name[0] == 'E' && parentDir->name[2] == 'M') {
        return;
    }

    std::string dirName = normalizedPath.substr(normalizedPath.find_last_of('/') + 1);
    
    if (dirName.size() > 2) {
        return;
    }

    if (parentDir->children.find(dirName) != parentDir->children.end()) {
        return;
    }

    Lump *startLump = new Lump{0, 0, dirName + "_START"};
    Lump *endLump = new Lump{0, 0, dirName + "_END"};

    if (parentDir == root) {
        lumps.push_back(startLump);
        lumps.push_back(endLump);
    } else {
        auto it = std::find_if(lumps.begin(), lumps.end(), [&](Lump *lump) {
            return lump->name == parentDir->name + "_END";
        });
        lumps.insert(it, startLump);
        lumps.insert(it, endLump);
    }

    Node *newDir = new Node(true, parentDir, dirName);
    parentDir->children[dirName] = newDir;
    parentDir->orderedChildren.push_back(dirName);

    descriptorCount += 2;
    saveWadFile();
}

void Wad::createFile(const std::string &path) {
    std::string normalizedPath = path;
    if (!normalizedPath.empty() && normalizedPath.back() == '/') {
        return;
    }

    std::string parentPath = normalizedPath.substr(0, normalizedPath.find_last_of('/'));
    if (parentPath.empty()) {
        parentPath = "/";
    }

    Node *parentDir = findNode(parentPath);
    if (!parentDir || !parentDir->isDirectory) {
        return;
    }

    if (parentDir->name.size() == 4 && parentDir->name[0] == 'E' && parentDir->name[2] == 'M') {
        return;
    }

    std::string fileName = normalizedPath.substr(normalizedPath.find_last_of('/') + 1);
    
    if (fileName.size() == 4 && fileName[0] == 'E' && fileName[2] == 'M' &&
        std::isdigit(fileName[1]) && std::isdigit(fileName[3])) {
        return;
    }

    if (parentDir->children.find(fileName) != parentDir->children.end()) {
        return;
    }

    if (fileName.size() > 8) {
        return;
    }

    Lump *newLump = new Lump{0, 0, fileName};

    if (parentDir == root) {
        lumps.push_back(newLump);
    } else {
        auto it = std::find_if(lumps.begin(), lumps.end(), [&](Lump *lump) {
            return lump->name == parentDir->name + "_END";
        });
        lumps.insert(it, newLump);
    }

    Node *newFile = new Node(false, parentDir, fileName);
    newFile->lump = newLump;
    parentDir->children[fileName] = newFile;
    parentDir->orderedChildren.push_back(fileName);
    descriptorCount++;
    saveWadFile();
}

int Wad::writeToFile(const std::string &path, const char *buffer, int length, int offset) {
    Node *node = findNode(path);
    if (!node || node->isDirectory) {
        return -1;
    }

    Lump *lump = node->lump;
    if (!lump) {
        return -1;
    }
    if (lump->size > 0) {
        return 0;
    }

    if (offset == 0){
        offset = descriptorOffset;
    }

    lump->size = length;
    lump->offset = offset;
    descriptorOffset += length;
    std::fstream file(wadPath, std::ios::binary | std::ios::in | std::ios::out);
    file.seekp(offset, std::ios::beg);
    file.write(buffer, length);
    file.flush();

    saveWadFile();
    return length;
}

void Wad::saveWadFile() {
    std::fstream file(wadPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open WAD file for writing: " << wadPath << std::endl;
        return;
    }

    file.seekp(4, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&descriptorCount), sizeof(int));
    file.write(reinterpret_cast<const char*>(&descriptorOffset), sizeof(int));

    std::vector<Lump*> orderedLumps;
    serializeNode(root, orderedLumps);

    file.seekp(descriptorOffset, std::ios::beg);
    for (Lump* lump : orderedLumps) {
        file.write(reinterpret_cast<const char*>(&lump->offset), sizeof(int));
        file.write(reinterpret_cast<const char*>(&lump->size), sizeof(int));
        char nameBuffer[8] = {0};
        std::strncpy(nameBuffer, lump->name.c_str(), 8);
        file.write(nameBuffer, 8);
    }

    file.flush();
    file.close();
}

void Wad::serializeNode(Node *node, std::vector<Lump*> &orderedLumps) const {
    if (!node) return;

    if (node != root) {
        auto startIt = std::find_if(lumps.begin(), lumps.end(), [&](Lump *lump) {
            return lump->name == node->name + "_START";
        });
        if (startIt != lumps.end()) {
            orderedLumps.push_back(*startIt);
        }
    }

    if (node->name.size() == 4 && node->name[0] == 'E' && node->name[2] == 'M') {
        auto mapMarkerIt = std::find_if(lumps.begin(), lumps.end(), [&](Lump *lump) {
            return lump->name == node->name;
        });
        if (mapMarkerIt != lumps.end()) {
            orderedLumps.push_back(*mapMarkerIt);
        }

        for (const auto &childName : node->orderedChildren) {
            Node *childNode = node->children.at(childName);
            if (!childNode->isDirectory && childNode->lump) {
                orderedLumps.push_back(childNode->lump);
            }
        }
    } else {
        for (const auto &childName : node->orderedChildren) {
            Node *childNode = node->children.at(childName);

            if (childNode->isDirectory) {
                serializeNode(childNode, orderedLumps);
            } else if (childNode->lump) {
                orderedLumps.push_back(childNode->lump);
            }
        }
    }
    if (node != root) {
        auto endIt = std::find_if(lumps.begin(), lumps.end(), [&](Lump *lump) {
            return lump->name == node->name + "_END";
        });
        if (endIt != lumps.end()) {
            orderedLumps.push_back(*endIt);
        }
    }
}