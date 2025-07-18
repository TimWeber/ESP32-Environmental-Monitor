#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>

/**
 * @brief Abstract calibration equation interface
 */
class ICalibrationEquation {
public:
    virtual ~ICalibrationEquation() = default;
    virtual float apply(float rawValue) const = 0;
    virtual std::string getType() const = 0;
    virtual std::string getDescription() const = 0;
};

/**
 * @brief Simple offset calibration (y = x + offset)
 */
class OffsetCalibration : public ICalibrationEquation {
private:
    float offset_;
    std::string unit_;

public:
    OffsetCalibration(float offset, const std::string& unit = "")
        : offset_(offset), unit_(unit) {}
    
    float apply(float rawValue) const override {
        return rawValue + offset_;
    }
    
    std::string getType() const override {
        return "offset";
    }
    
    std::string getDescription() const override {
        return "y = x + " + std::to_string(offset_) + " " + unit_;
    }
    
    float getOffset() const { return offset_; }
    const std::string& getUnit() const { return unit_; }
};

/**
 * @brief Linear calibration (y = mx + b)
 */
class LinearCalibration : public ICalibrationEquation {
private:
    float slope_;
    float intercept_;
    std::string unit_;

public:
    LinearCalibration(float slope, float intercept, const std::string& unit = "")
        : slope_(slope), intercept_(intercept), unit_(unit) {}
    
    float apply(float rawValue) const override {
        return slope_ * rawValue + intercept_;
    }
    
    std::string getType() const override {
        return "linear";
    }
    
    std::string getDescription() const override {
        return "y = " + std::to_string(slope_) + "x + " + std::to_string(intercept_) + " " + unit_;
    }
    
    float getSlope() const { return slope_; }
    float getIntercept() const { return intercept_; }
    const std::string& getUnit() const { return unit_; }
};

/**
 * @brief Polynomial calibration (y = ax² + bx + c)
 */
class PolynomialCalibration : public ICalibrationEquation {
private:
    std::vector<float> coefficients_;
    std::string unit_;

public:
    PolynomialCalibration(const std::vector<float>& coefficients, const std::string& unit = "")
        : coefficients_(coefficients), unit_(unit) {}
    
    float apply(float rawValue) const override {
        float result = 0.0f;
        float power = 1.0f;
        
        for (float coeff : coefficients_) {
            result += coeff * power;
            power *= rawValue;
        }
        
        return result;
    }
    
    std::string getType() const override {
        return "polynomial";
    }
    
    std::string getDescription() const override {
        std::string desc = "y = ";
        for (size_t i = 0; i < coefficients_.size(); ++i) {
            if (i > 0) desc += " + ";
            desc += std::to_string(coefficients_[i]);
            if (i > 0) desc += "x^" + std::to_string(i);
        }
        desc += " " + unit_;
        return desc;
    }
    
    const std::vector<float>& getCoefficients() const { return coefficients_; }
    const std::string& getUnit() const { return unit_; }
};

/**
 * @brief Custom calibration using a function pointer
 */
class CustomCalibration : public ICalibrationEquation {
private:
    std::function<float(float)> function_;
    std::string description_;
    std::string unit_;

public:
    CustomCalibration(std::function<float(float)> func, const std::string& description, const std::string& unit = "")
        : function_(func), description_(description), unit_(unit) {}
    
    float apply(float rawValue) const override {
        return function_(rawValue);
    }
    
    std::string getType() const override {
        return "custom";
    }
    
    std::string getDescription() const override {
        return description_ + " " + unit_;
    }
    
    const std::string& getUnit() const { return unit_; }
};

/**
 * @brief Calibration manager for sensor data
 */
class CalibrationManager {
private:
    std::unique_ptr<ICalibrationEquation> temperatureCalibration_;
    std::unique_ptr<ICalibrationEquation> humidityCalibration_;
    std::unique_ptr<ICalibrationEquation> eco2Calibration_;
    std::unique_ptr<ICalibrationEquation> tvocCalibration_;

public:
    CalibrationManager() = default;
    
    // Set calibration equations
    void setTemperatureCalibration(std::unique_ptr<ICalibrationEquation> cal) {
        temperatureCalibration_ = std::move(cal);
    }
    
    void setHumidityCalibration(std::unique_ptr<ICalibrationEquation> cal) {
        humidityCalibration_ = std::move(cal);
    }
    
    void setEco2Calibration(std::unique_ptr<ICalibrationEquation> cal) {
        eco2Calibration_ = std::move(cal);
    }
    
    void setTvocCalibration(std::unique_ptr<ICalibrationEquation> cal) {
        tvocCalibration_ = std::move(cal);
    }
    
    // Apply calibrations
    float calibrateTemperature(float rawValue) const {
        if (temperatureCalibration_) {
            return temperatureCalibration_->apply(rawValue);
        }
        return rawValue;
    }
    
    float calibrateHumidity(float rawValue) const {
        if (humidityCalibration_) {
            return humidityCalibration_->apply(rawValue);
        }
        return rawValue;
    }
    
    float calibrateEco2(float rawValue) const {
        if (eco2Calibration_) {
            return eco2Calibration_->apply(rawValue);
        }
        return rawValue;
    }
    
    float calibrateTvoc(float rawValue) const {
        if (tvocCalibration_) {
            return tvocCalibration_->apply(rawValue);
        }
        return rawValue;
    }
    
    // Get calibration info
    std::string getTemperatureCalibrationInfo() const {
        return temperatureCalibration_ ? temperatureCalibration_->getDescription() : "No calibration";
    }
    
    std::string getHumidityCalibrationInfo() const {
        return humidityCalibration_ ? humidityCalibration_->getDescription() : "No calibration";
    }
    
    std::string getEco2CalibrationInfo() const {
        return eco2Calibration_ ? eco2Calibration_->getDescription() : "No calibration";
    }
    
    std::string getTvocCalibrationInfo() const {
        return tvocCalibration_ ? tvocCalibration_->getDescription() : "No calibration";
    }
}; 