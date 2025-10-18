#pragma once

#include <ArduinoJson.h>
#include <vector>

/**
 * Utility class for finding sensors in Libre Hardware Monitor JSON structure.
 * Provides flexible matching for different hardware configurations.
 */
class SensorFinder {
 public:
    /**
     * Find a sensor by exact text match
     */
    static JsonObject findByExactName(JsonArray sensors, const char* name);

    /**
     * Find a sensor by checking if its Text field contains any of the patterns
     */
    static JsonObject findByPartialMatch(JsonArray sensors,
                                         const std::vector<const char*>& patterns);

    /**
     * Find a section (e.g., "Temperatures", "Load") within hardware children
     */
    static JsonObject findSection(JsonArray children, const char* sectionName);

    /**
     * Find first sensor containing the substring in its Text field
     */
    static JsonObject findContaining(JsonArray sensors, const char* substring);

 private:
    static bool textContains(const char* text, const char* substring);
    static bool textMatches(const char* text, const std::vector<const char*>& patterns);
};