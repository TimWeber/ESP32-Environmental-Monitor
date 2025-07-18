#include "json_config_provider.hpp"
#include "esp_log.h"
#include <cstring>

static const char* TAG = "JsonConfigProvider";

JsonConfigProvider::JsonConfigProvider()
    : rootJson_(nullptr), currentJson_(nullptr), configPath_("") {
    // Default constructor for section creation - no initialisation needed
}

JsonConfigProvider::JsonConfigProvider(const std::string& configPath)
    : rootJson_(nullptr), currentJson_(nullptr), configPath_(configPath) {
    
    ESP_LOGI(TAG, "Creating JSON config provider for: %s", configPath_.c_str());
    
    if (!loadJsonFile()) {
        ESP_LOGE(TAG, "Failed to load JSON file: %s", configPath_.c_str());
        return;
    }
    
    currentJson_ = rootJson_;  // Start with root
    ESP_LOGI(TAG, "JSON config provider initialised successfully");
}

JsonConfigProvider::~JsonConfigProvider() {
    // Only delete rootJson_ if this provider owns it (not a section provider)
    if (rootJson_) {
        cJSON_Delete(rootJson_);
        rootJson_ = nullptr;
    }
    currentJson_ = nullptr;
}

bool JsonConfigProvider::loadJsonFile() {
    ESP_LOGI(TAG, "Loading JSON file: %s", configPath_.c_str());
    
    FILE* file = fopen(configPath_.c_str(), "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open config file: %s", configPath_.c_str());
        return false;
    }
    
    ESP_LOGI(TAG, "Successfully opened config file: %s", configPath_.c_str());
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for config file");
        fclose(file);
        return false;
    }
    
    size_t bytesRead = fread(buffer, 1, size, file);
    if (bytesRead != static_cast<size_t>(size)) {
        ESP_LOGE(TAG, "Failed to read complete config file. Expected %ld bytes, read %zu", size, bytesRead);
        free(buffer);
        fclose(file);
        return false;
    }
    buffer[size] = '\0';
    fclose(file);
    
    rootJson_ = cJSON_Parse(buffer);
    free(buffer);
    
    if (!rootJson_) {
        ESP_LOGE(TAG, "Failed to parse JSON file: %s", configPath_.c_str());
        return false;
    }
    
    ESP_LOGI(TAG, "JSON file parsed successfully");
    return true;
}

bool JsonConfigProvider::getString(const std::string& key, std::string& value) const {
    cJSON* jsonValue = findJsonValue(key);
    if (jsonValue && cJSON_IsString(jsonValue)) {
        value = std::string(jsonValue->valuestring);
        return true;
    }
    return false;
}

bool JsonConfigProvider::getInt(const std::string& key, int& value) const {
    cJSON* jsonValue = findJsonValue(key);
    if (jsonValue && cJSON_IsNumber(jsonValue)) {
        value = jsonValue->valueint;
        return true;
    }
    return false;
}

bool JsonConfigProvider::getUInt(const std::string& key, uint32_t& value) const {
    cJSON* jsonValue = findJsonValue(key);
    if (jsonValue && cJSON_IsNumber(jsonValue)) {
        value = static_cast<uint32_t>(jsonValue->valueint);
        return true;
    }
    return false;
}

bool JsonConfigProvider::getBool(const std::string& key, bool& value) const {
    cJSON* jsonValue = findJsonValue(key);
    if (jsonValue && cJSON_IsBool(jsonValue)) {
        value = cJSON_IsTrue(jsonValue);
        return true;
    }
    return false;
}

bool JsonConfigProvider::getFloat(const std::string& key, float& value) const {
    cJSON* jsonValue = findJsonValue(key);
    if (jsonValue && cJSON_IsNumber(jsonValue)) {
        value = static_cast<float>(jsonValue->valuedouble);
        return true;
    }
    return false;
}

std::unique_ptr<IConfigProvider> JsonConfigProvider::getSection(const std::string& section) const {
    if (!currentJson_) {
        ESP_LOGW(TAG, "No current JSON context available");
        return nullptr;
    }
    
    cJSON* sectionJson = cJSON_GetObjectItem(currentJson_, section.c_str());
    
    if (!sectionJson) {
        ESP_LOGW(TAG, "Section '%s' not found", section.c_str());
        return nullptr;
    }
    
    if (!cJSON_IsObject(sectionJson)) {
        ESP_LOGW(TAG, "Section '%s' found but is not an object", section.c_str());
        return nullptr;
    }
    
    // Create a new provider that points to this section
    auto sectionProvider = std::make_unique<JsonConfigProvider>();
    
    // Section providers don't own the root to prevent corruption
    sectionProvider->rootJson_ = nullptr;
    sectionProvider->currentJson_ = sectionJson;
    sectionProvider->configPath_ = configPath_;
    
    return sectionProvider;
}

bool JsonConfigProvider::hasKey(const std::string& key) const {
    if (!currentJson_) {
        return false;
    }
    
    cJSON* jsonValue = cJSON_GetObjectItem(currentJson_, key.c_str());
    return jsonValue != nullptr;
}

bool JsonConfigProvider::hasSection(const std::string& section) const {
    if (!currentJson_) {
        return false;
    }
    
    cJSON* sectionJson = cJSON_GetObjectItem(currentJson_, section.c_str());
    return sectionJson != nullptr && cJSON_IsObject(sectionJson);
}

cJSON* JsonConfigProvider::findJsonValue(const std::string& key) const {
    if (!currentJson_) {
        return nullptr;
    }
    
    return cJSON_GetObjectItem(currentJson_, key.c_str());
} 