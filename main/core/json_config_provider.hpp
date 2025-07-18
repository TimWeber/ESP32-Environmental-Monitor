#pragma once

#include "config_manager.hpp"
#include "cJSON.h"
#include <string>
#include <memory>

/**
 * @brief JSON-specific implementation of IConfigProvider
 * 
 * This class handles JSON configuration files using cJSON library.
 * It's completely isolated from the rest of the system.
 */
class JsonConfigProvider : public IConfigProvider {
public:
    JsonConfigProvider();  // Default constructor for section creation
    explicit JsonConfigProvider(const std::string& configPath);
    ~JsonConfigProvider() override;
    
    // IConfigProvider interface implementation
    bool getString(const std::string& key, std::string& value) const override;
    bool getInt(const std::string& key, int& value) const override;
    bool getUInt(const std::string& key, uint32_t& value) const override;
    bool getBool(const std::string& key, bool& value) const override;
    bool getFloat(const std::string& key, float& value) const override;
    
    std::unique_ptr<IConfigProvider> getSection(const std::string& section) const override;
    bool hasKey(const std::string& key) const override;
    bool hasSection(const std::string& section) const override;
    
private:
    cJSON* rootJson_;
    cJSON* currentJson_;
    std::string configPath_;
    
    // Helper methods
    bool loadJsonFile();
    cJSON* findJsonValue(const std::string& key) const;
}; 