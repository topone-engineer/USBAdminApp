#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <usbiodef.h>
#include <iostream>
#include <string>

#pragma comment(lib, "setupapi.lib")

static std::string WStringToString(const std::wstring& wstr) {
    return std::string(wstr.begin(), wstr.end());
}

static HANDLE FindDevice(const std::string& vid, const std::string& pid) {
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get device info set, error: " << GetLastError() << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (int i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
        char buffer[256];
        SetupDiGetDeviceInstanceIdA(deviceInfoSet, &deviceInfoData, buffer, sizeof(buffer), NULL);
        std::string deviceID = buffer;

        if (deviceID.find(vid) != std::string::npos && deviceID.find(pid) != std::string::npos) {
            std::cout << "Device found: " << deviceID << std::endl;
            SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
            deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            if (SetupDiEnumDeviceInterfaces(deviceInfoSet, &deviceInfoData, &GUID_DEVINTERFACE_USB_DEVICE, 0, &deviceInterfaceData)) {
                DWORD requiredSize = 0;
                SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

                SP_DEVICE_INTERFACE_DETAIL_DATA_A* deviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(requiredSize);
                if (deviceInterfaceDetailData == NULL) {
                    std::cerr << "Failed to allocate memory for deviceInterfaceDetailData" << std::endl;
                    SetupDiDestroyDeviceInfoList(deviceInfoSet);
                    return INVALID_HANDLE_VALUE;
                }

                deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

                if (SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, &requiredSize, &deviceInfoData)) {
                    std::string devicePath = deviceInterfaceDetailData->DevicePath;
                    std::cout << "Device Path: " << devicePath << std::endl;  // Log the device path

                    free(deviceInterfaceDetailData);
                    SetupDiDestroyDeviceInfoList(deviceInfoSet);

                    HANDLE hDevice = CreateFileA(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hDevice == INVALID_HANDLE_VALUE) {
                        std::cerr << "Failed to open device, error: " << GetLastError() << std::endl;
                    }

                    return hDevice;
                }

                free(deviceInterfaceDetailData);
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    std::cerr << "Device not found" << std::endl;
    return INVALID_HANDLE_VALUE;
}

void CommunicateWithDevice(HANDLE hDevice) {
    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open device" << std::endl;
        return;
    }

    char writeBuffer[] = "Hello, ESP32-C3!";
    DWORD bytesWritten;
    if (!WriteFile(hDevice, writeBuffer, sizeof(writeBuffer), &bytesWritten, NULL)) {
        std::cerr << "Failed to write to device, error: " << GetLastError() << std::endl;
        CloseHandle(hDevice);
        return;
    }
    std::cout << "Bytes written: " << bytesWritten << std::endl;

    char readBuffer[256];
    DWORD bytesRead;
    if (ReadFile(hDevice, readBuffer, sizeof(readBuffer) - 1, &bytesRead, NULL)) {
        readBuffer[bytesRead] = '\0';
        std::cout << "Received: " << readBuffer << std::endl;
    }
    else {
        std::cerr << "Failed to read from device, error: " << GetLastError() << std::endl;
    }

    CloseHandle(hDevice);
}

int main() {
    std::string vid = "VID_303A"; // Replace with your actual VID
    std::string pid = "PID_1001"; // Replace with your actual PID

    HANDLE hDevice = FindDevice(vid, pid);
    CommunicateWithDevice(hDevice);

    return 0;
}
