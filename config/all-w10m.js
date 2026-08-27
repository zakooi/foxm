// ==============================================================================
// FoxM - Engine Preferences Tuned for Low-RAM Windows 10 Mobile Devices
// ==============================================================================

// 1. Quản lý Bộ nhớ & Cache (Memory & Cache Management)
pref("browser.cache.memory.enable", true);
pref("browser.cache.memory.capacity", 16384);          // Giới hạn Cache RAM tối đa 16MB
pref("browser.cache.disk.enable", true);
pref("browser.cache.disk.capacity", 1048576);         // Cache trên bộ nhớ flash 1GB
pref("image.mem.max_decoded_image_kb", 32768);         // Giữ tối đa 32MB ảnh giải mã
pref("browser.sessionhistory.max_entries", 4);          // Chỉ giữ 4 bước Back/Forward
pref("browser.sessionhistory.max_total_viewers", 1);    // Tắt Fastback bfcache để tiết kiệm RAM

// 2. SpiderMonkey Garbage Collection & JIT
pref("javascript.options.mem.gc_frequency", 50);        // Dọn rác thường xuyên
pref("javascript.options.mem.high_water_mark", 64);     // Ngưỡng RAM JS tối đa 64MB
pref("javascript.options.baselinejit", true);
pref("javascript.options.ion", false);                  // Tắt Ion JIT nặng nề để tiết kiệm RAM

// 3. Hiển thị & Cuộn trang (Viewport & Scrolling)
pref("layout.css.flexbox.enabled", true);
pref("layout.css.grid.enabled", true);
pref("image.webp.enabled", true);
pref("image.avif.enabled", false);                      // Tắt AVIF để tránh quá tải CPU ARMv7
pref("layers.acceleration.force-enabled", true);
pref("layers.d3d11.enabled", true);
pref("layers.offmainthreadcomposition.enabled", true);

// 4. Mạng & Bảo mật (Network & Security)
pref("security.tls.version.min", 3);                    // Tối thiểu TLS 1.2 (Bảo mật tối đa, chặn BEAST)
pref("security.tls.version.max", 4);                    // Hỗ trợ TLS 1.3 (NSS Core)
pref("network.http.pipelining", false);                 // Tắt pipelining tránh lỗi tương thích CDN
pref("network.http.max-connections", 32);
pref("network.http.max-connections-per-server", 6);
pref("network.dns.disableIPv6", true);                  // Giảm độ trễ DNS trên mạng di động 3G/4G

// 5. User-Agent di động chuẩn hiện đại (Gecko Mobile)
pref("general.useragent.override", "Mozilla/5.0 (Android; Mobile; rv:109.0) Gecko/115.0 Firefox/115.0");
