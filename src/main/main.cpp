#include <iostream>
#include <exception>
#include <cstdlib>
import CustomVulkan.Encapsulation;

int main() {
    try {
        CustomVulkan::VulkanInit::run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
