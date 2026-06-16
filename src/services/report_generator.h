/**
 * @file report_generator.h
 * @brief Report Generator - creates HTML diagnostic reports on SD card.
 */

#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include <Arduino.h>
#include <SD.h>
#include <vector>
#include "config_types.h"
#include "services/health_score.h"
#include "services/freeze_frame.h"
#include "services/cranking_analyzer.h"
#include "core/protocol_plugin.h"

class ReportGenerator {
public:
    ReportGenerator() : reportCounter(0) {}

    /**
     * Generate an HTML diagnostic report and save to SD card.
     * Returns the filename of the generated report.
     */
    String generate(const VehicleProfile& profile, const HealthReport& health,
                    const std::vector<DTC>& dtcs, const std::vector<FreezeFrame>& frames,
                    const std::vector<CrankEvent>& crankEvents,
                    float currentBatt, float currentRPM, float currentTemp) {
        
        reportCounter++;
        String filename = "/reports/report_" + String(reportCounter) + ".html";

        // Ensure directory exists
        if (!SD.exists("/reports")) {
            SD.mkdir("/reports");
        }

        File file = SD.open(filename, FILE_WRITE);
        if (!file) {
            Serial.println("[Report] Failed to create: " + filename);
            return "";
        }

        // Write HTML header
        file.println("<!DOCTYPE html><html><head>");
        file.println("<meta charset='UTF-8'>");
        file.println("<meta name='viewport' content='width=device-width,initial-scale=1'>");
        file.println("<title>Laporan Diagnostik - " + profile.name + "</title>");
        file.println("<style>");
        writeCSS(file);
        file.println("</style></head><body>");

        // Header
        file.println("<div class='header'>");
        file.println("<h1>Laporan Diagnostik Motor</h1>");
        file.println("<p>Smart FI Diagnostic Tool</p>");
        file.println("</div>");

        // Vehicle Info
        file.println("<div class='section'>");
        file.println("<h2>Informasi Kendaraan</h2>");
        file.println("<table>");
        file.println("<tr><td>Nama</td><td>" + profile.name + "</td></tr>");
        file.println("<tr><td>Brand</td><td>" + profile.brand + "</td></tr>");
        file.println("<tr><td>Family</td><td>" + profile.family + "</td></tr>");
        file.println("<tr><td>Tier</td><td>" + String(profile.tier) + "</td></tr>");
        file.println("</table></div>");

        // Health Score
        file.println("<div class='section'>");
        file.println("<h2>Skor Kesehatan</h2>");
        file.println("<div class='score " + getScoreClass(health.overall_score) + "'>");
        file.println(String(health.overall_score, 1) + "%");
        file.println("</div>");
        file.println("<p>" + health.summary + "</p>");
        file.println("<table>");
        file.println("<tr><th>Komponen</th><th>Skor</th></tr>");
        file.println("<tr><td>Baterai</td><td>" + String(health.battery_score, 0) + "%</td></tr>");
        file.println("<tr><td>TPS</td><td>" + String(health.tps_score, 0) + "%</td></tr>");
        file.println("<tr><td>Idle</td><td>" + String(health.idle_score, 0) + "%</td></tr>");
        file.println("<tr><td>Charging</td><td>" + String(health.charging_score, 0) + "%</td></tr>");
        file.println("<tr><td>Thermal</td><td>" + String(health.thermal_score, 0) + "%</td></tr>");
        file.println("</table></div>");

        // Current Readings
        file.println("<div class='section'>");
        file.println("<h2>Pembacaan Saat Ini</h2>");
        file.println("<table>");
        file.println("<tr><td>Baterai</td><td>" + String(currentBatt, 2) + " V</td></tr>");
        file.println("<tr><td>RPM</td><td>" + String(currentRPM, 0) + "</td></tr>");
        file.println("<tr><td>Suhu</td><td>" + String(currentTemp, 1) + " &deg;C</td></tr>");
        file.println("</table></div>");

        // DTC Codes
        if (!dtcs.empty()) {
            file.println("<div class='section'>");
            file.println("<h2>Kode Error (DTC)</h2>");
            file.println("<table><tr><th>Kode</th><th>Deskripsi</th><th>Status</th></tr>");
            for (const auto& dtc : dtcs) {
                String status = dtc.is_active ? "<span class='red'>AKTIF</span>" : "Historis";
                file.println("<tr><td>" + dtc.code + "</td><td>" + dtc.description + "</td><td>" + status + "</td></tr>");
            }
            file.println("</table></div>");
        }

        // Freeze Frames
        if (!frames.empty()) {
            file.println("<div class='section'>");
            file.println("<h2>Freeze Frame Data</h2>");
            file.println("<table><tr><th>Trigger</th><th>Batt</th><th>RPM</th><th>TPS</th><th>Temp</th></tr>");
            for (const auto& f : frames) {
                file.println("<tr><td>" + f.trigger + "</td>");
                file.println("<td>" + String(f.battery_v, 2) + "V</td>");
                file.println("<td>" + String(f.rpm, 0) + "</td>");
                file.println("<td>" + String(f.tps_pct, 1) + "%</td>");
                file.println("<td>" + String(f.temperature, 1) + "C</td></tr>");
            }
            file.println("</table></div>");
        }

        // Cranking Events
        if (!crankEvents.empty()) {
            file.println("<div class='section'>");
            file.println("<h2>Analisis Starter</h2>");
            file.println("<table><tr><th>Durasi</th><th>V Min</th><th>Drop</th><th>Hasil</th><th>Verdict</th></tr>");
            for (const auto& ev : crankEvents) {
                file.println("<tr><td>" + String(ev.duration_ms) + "ms</td>");
                file.println("<td>" + String(ev.min_voltage, 2) + "V</td>");
                file.println("<td>" + String(ev.voltage_drop, 2) + "V</td>");
                file.println("<td>" + String(ev.engine_started ? "START" : "GAGAL") + "</td>");
                file.println("<td>" + ev.verdict + "</td></tr>");
            }
            file.println("</table></div>");
        }

        // Footer
        file.println("<div class='footer'>");
        file.println("<p>Generated by Smart FI Diagnostic Tool</p>");
        file.println("<p>Uptime: " + String(millis() / 1000) + " detik</p>");
        file.println("</div>");

        file.println("</body></html>");
        file.close();

        Serial.println("[Report] Generated: " + filename);
        return filename;
    }

private:
    int reportCounter;

    void writeCSS(File& file) {
        file.println("body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;}");
        file.println(".header{background:#1a73e8;color:white;padding:20px;border-radius:8px;margin-bottom:20px;}");
        file.println(".section{background:white;padding:15px;border-radius:8px;margin-bottom:15px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}");
        file.println("table{width:100%;border-collapse:collapse;margin-top:10px;}");
        file.println("td,th{padding:8px;border-bottom:1px solid #eee;text-align:left;}");
        file.println("th{background:#f8f9fa;font-weight:bold;}");
        file.println(".score{font-size:48px;font-weight:bold;text-align:center;padding:20px;}");
        file.println(".good{color:#34a853;}.warn{color:#fbbc04;}.bad{color:#ea4335;}");
        file.println(".red{color:#ea4335;font-weight:bold;}");
        file.println(".footer{text-align:center;color:#666;padding:20px;font-size:12px;}");
    }

    String getScoreClass(float score) {
        if (score >= 70) return "good";
        if (score >= 40) return "warn";
        return "bad";
    }
};

#endif // REPORT_GENERATOR_H
