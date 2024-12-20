#ifndef WAD_H
#define WAD_H

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <stdexcept>

class Wad {
public:
    static Wad* loadWad(const std::string &path);
    ~Wad();

    std::string getMagic() const;
    bool isContent(const std::string &path) const;
    bool isDirectory(const std::string &path) const;
    int getSize(const std::string &path) const;
    int getContents(const std::string &path, char *buffer, int length, int offset = 0) const;
    int getDirectory(const std::string &path, std::vector<std::string> *directory) const;

    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char *buffer, int length, int offset = 0);

private:
    struct Lump {
        size_t offset;
        size_t size;
        std::string name;

        Lump(size_t offset = 0, size_t size = 0, const std::string& name = "")
            : offset(offset), size(size), name(name) {}
    };

    struct Node {
        bool isDirectory;
        Lump *lump;
        std::unordered_map<std::string, Node*> children;
        std::vector<std::string> orderedChildren;
        Node* parent;
        std::string name;

        Node(bool isDir, Node* parent = nullptr, const std::string& name = "")
            : isDirectory(isDir), lump(nullptr), parent(parent), name(name) {}

        ~Node() {
            for (auto &child : children) {
                delete child.second;
            }
        }
    };

    Wad();
    void parseWadFile(const std::string &path);
    void buildDirectoryStructure();
    Node* findNode(const std::string &path) const;
    void saveWadFile();
    void serializeNode(Node *node, std::vector<Lump*> &orderedLumps) const;
    
    std::string magic;
    int descriptorCount;
    int descriptorOffset;
    std::string wadPath;
    std::vector<Lump*> lumps;
    Node* root;
};

#endif // WAD_H