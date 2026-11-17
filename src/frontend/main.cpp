#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--version") {
        std::cout << "NEXUS PS5 0.1.0\n";
        return 0;
    }

    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << "Usage: nexus [--help|--version]\n";
        return 0;
    }

    if (argc != 1) {
        std::cerr << "Unsupported argument. Use --help for available options.\n";
        return 2;
    }

    std::cout << "NEXUS PS5 development foundation; emulator runtime is not implemented.\n";
    return 0;
}
