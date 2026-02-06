#include <openxr/openxr.h>
#include <iostream>

int main() {
    uint32_t extensionCount = 0;
    XrResult result = xrEnumerateInstanceExtensionProperties(
        nullptr, 0, &extensionCount, nullptr);

    std::cout << "OpenXR SDK linked successfully" << std::endl;
    std::cout << "xrEnumerateInstanceExtensionProperties returned: "
              << result << std::endl;
    std::cout << "Extension count: " << extensionCount << std::endl;
    return 0;
}
