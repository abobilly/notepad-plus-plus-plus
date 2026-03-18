#!/usr/bin/env node

/**
 * svgrepo-scrape.js
 *
 * Example scraper for SVG Repo collections.
 *
 * Usage:
 *   node scripts/svgrepo-scrape.js --out ./svgs
 *
 * By default it will scrape a curated set of collections (from the task prompt)
 * and download all SVGs into the output directory.
 *
 * Notes:
 * - svgrepo.com may rate-limit/block automated requests (HTTP 429). If you see
 *   429 responses, try running this from a different network / IP, or add
 *   a small delay between requests.
 * - This is intended to be executed locally where you have normal browser access.
 */

const fs = require("fs");
const path = require("path");
const { setTimeout: delay } = require("timers/promises");
const pLimitModule = require("p-limit");
const pLimit = typeof pLimitModule === "function" ? pLimitModule : pLimitModule.default || pLimitModule;
const cheerio = require("cheerio");

const DEFAULT_COLLECTIONS = [
  {
    name: "Chunk 16px Thick Interface Icons",
    baseUrl: "https://www.svgrepo.com/collection/chunk-16px-thick-interface-icons/",
    pages: 13,
  },
  {
    name: "Business Sharp Line Duotone Icons",
    baseUrl: "https://www.svgrepo.com/collection/business-sharp-line-duotone-icons/",
  },
  {
    name: "Internet Duotone Line Icons",
    baseUrl: "https://www.svgrepo.com/collection/internet-duotone-line-icons/",
  },
  {
    name: "Isometric 3d Interface Icons",
    baseUrl: "https://www.svgrepo.com/collection/isometric-3d-interface-icons/",
  },
  {
    name: "Office Duotone Minimalist Icons",
    baseUrl: "https://www.svgrepo.com/collection/office-duotone-minimalist-icons/",
  },
  {
    name: "Business Duotone Tiny Icons",
    baseUrl: "https://www.svgrepo.com/collection/business-duotone-tiny-icons/",
  },
  {
    name: "Business Management Flat Vectors",
    baseUrl: "https://www.svgrepo.com/collection/business-management-flat-vectors/",
  },
  {
    name: "Bigmug (B Collection)",
    baseUrl: "https://www.svgrepo.com/collection/bigmug-interface-icons/",
    // may have multiple pages; scraper will auto-discover
  },
  {
    name: "Gentlecons (G Collection)",
    baseUrl: "https://www.svgrepo.com/collection/gentlecons-interface-icons/",
  },
  {
    name: "Iconship (I Collection)",
    baseUrl: "https://www.svgrepo.com/collection/iconship-interface-icons/",
  },
  {
    name: "Mobile App Essentials (M Collection)",
    baseUrl: "https://www.svgrepo.com/collection/mobile-app-essentials-icons/",
  },
  {
    name: "Neuicons (N Collection)",
    baseUrl: "https://www.svgrepo.com/collection/neuicons-oval-line-icons/",
  },
  {
    name: "Stylized Dashboard (S Collection)",
    baseUrl: "https://www.svgrepo.com/collection/stylized-dashboard-icons/",
  },
  {
    name: "Xnix Circular Interface Icons (X Collection)",
    baseUrl: "https://www.svgrepo.com/collection/xnix-circular-interface-icons/",
  },
  {
    name: "Nuiverse Sharp Interface Icons",
    baseUrl: "https://www.svgrepo.com/collection/nuiverse-sharp-interface-icons/",
  },
  {
    name: "Emoji Face Emoji Vectors",
    baseUrl: "https://www.svgrepo.com/collection/emoji-face-emoji-vectors/",
  },
  {
    name: "Daily Life Sepia Icons",
    baseUrl: "https://www.svgrepo.com/collection/daily-life-sepia-icons/",
  },
  {
    name: "Outlined Business Vectors",
    baseUrl: "https://www.svgrepo.com/collection/outlined-business-vectors/",
  },
  {
    name: "Ligature Symbols Icons",
    baseUrl: "https://www.svgrepo.com/collection/ligature-symbols-icons/3",
  },
  {
    name: "Capitaine Cursor Vectors",
    baseUrl: "https://www.svgrepo.com/collection/capitaine-cursor-vectors/",
  },
  {
    name: "Iconhub Tritone Icons",
    baseUrl: "https://www.svgrepo.com/collection/iconhub-tritone-icons/",
  },
  {
    name: "Stylized Bordered Interface Icons",
    baseUrl: "https://www.svgrepo.com/collection/stylized-bordered-interface-icons/",
  },
];

const DEFAULT_OUTPUT_DIR = path.resolve(process.cwd(), "svgrepo-icons");

function normalizeUrl(url) {
  return url.replace(/\s+/g, "").trim();
}

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

async function fetchWithRetry(url, opts = {}, maxRetries = 6) {
  const headers = {
    "User-Agent":
      "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/127.0.0.0 Safari/537.36",
    Accept: "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    ...opts.headers,
  };

  for (let attempt = 1; attempt <= maxRetries; ++attempt) {
    try {
      const res = await fetch(url, { ...opts, headers });
      if (res.status === 429) {
        const waitMs = 1000 * attempt;
        console.warn(`Received 429 for ${url} (attempt ${attempt}), waiting ${waitMs}ms`);
        await delay(waitMs);
        continue;
      }
      if (!res.ok) {
        throw new Error(`HTTP ${res.status} (${res.statusText}) for ${url}`);
      }
      return res;
    } catch (err) {
      if (attempt === maxRetries) throw err;
      const waitMs = 1000 * attempt;
      console.warn(`Fetch failed for ${url} (attempt ${attempt}): ${err.message}. Retrying in ${waitMs}ms...`);
      await delay(waitMs);
    }
  }
  throw new Error(`Failed to fetch ${url} after ${maxRetries} attempts`);
}

function cleanFileName(name) {
  return name
    .replace(/[\s\/\\:*?"<>|]/g, "-")
    .replace(/-+/g, "-")
    .replace(/^-+|-+$/g, "")
    .slice(0, 150);
}

function ensureExtension(fileName, ext) {
  if (!fileName.toLowerCase().endsWith(ext.toLowerCase())) {
    return fileName + ext;
  }
  return fileName;
}

async function scrapeCollection(collection, outDir, opts = {}) {
  const collectionDir = path.join(outDir, cleanFileName(collection.name));
  ensureDir(collectionDir);

  const seenSvgUrls = new Set();
  const downloaded = [];
  const errors = [];

  const pages = collection.pages || 1;
  let pageUrls = [];

  if (collection.pages) {
    for (let i = 1; i <= pages; ++i) {
      if (i === 1) pageUrls.push(collection.baseUrl);
      else pageUrls.push(new URL(`${i}`, collection.baseUrl).href);
    }
  } else {
    // will discover pages dynamically
    pageUrls.push(collection.baseUrl);
  }

  const limit = pLimit(opts.concurrency || 2);

  async function fetchPageAndExtract(pageUrl) {
    const res = await fetchWithRetry(pageUrl, { redirect: "follow" });
    const html = await res.text();
    const $ = cheerio.load(html);

    // Attempt to discover pagination links, if we don't already have fixed pages.
    if (!collection.pages) {
      const paginationLinks = $(".pagination a, .pagination li a")
        .map((_, el) => $(el).attr("href"))
        .get()
        .filter(Boolean)
        .map((href) => new URL(href, pageUrl).href);

      for (const p of paginationLinks) {
        if (!pageUrls.includes(p)) {
          pageUrls.push(p);
        }
      }
    }

    // Identify icon detail links (links pointing at /svg/<id>/...)
    const iconLinks = $("a[href*='/svg/']")
      .map((_, el) => $(el).attr("href"))
      .get()
      .filter(Boolean)
      .map((href) => new URL(href, pageUrl).href);

    return iconLinks;
  }

  async function fetchSvgFromIconPage(iconPageUrl) {
    const match = iconPageUrl.match(/\/svg\/(\d+)(?:\/([^/?#]+))?/);
    const iconId = match?.[1];
    const iconNameFromUrl = match?.[2];

    const downloadUrl = iconId
      ? `https://www.svgrepo.com/download/svg/${iconId}`
      : null;

    const svgUrl = downloadUrl || iconPageUrl;
    if (!svgUrl) {
      throw new Error(`Cannot determine svg URL for ${iconPageUrl}`);
    }

    if (seenSvgUrls.has(svgUrl)) {
      return null;
    }
    seenSvgUrls.add(svgUrl);

    const res = await fetchWithRetry(svgUrl, { redirect: "follow" });

    // Some endpoints return a redirect to the actual svg file.
    const finalUrl = res.url;
    const text = await res.text();

    // Determine filename
    let fileName = iconNameFromUrl || path.basename(new URL(finalUrl).pathname) || iconId || "icon";
    fileName = cleanFileName(fileName);
    fileName = ensureExtension(fileName, ".svg");

    const outPath = path.join(collectionDir, fileName);
    await fs.promises.writeFile(outPath, text, "utf8");

    return { iconPageUrl, svgUrl: finalUrl, outPath };
  }

  const pageQueue = [...pageUrls];
  const seenPages = new Set();
  while (pageQueue.length > 0) {
    const pageUrl = pageQueue.shift();
    if (seenPages.has(pageUrl)) continue;
    seenPages.add(pageUrl);

    console.log(`Scraping collection '${collection.name}' page: ${pageUrl}`);
    let iconLinks = [];
    try {
      iconLinks = await fetchPageAndExtract(pageUrl);
    } catch (err) {
      errors.push({ type: "page", pageUrl, error: err.message });
      console.warn(`Failed to scrape page ${pageUrl}: ${err.message}`);
      continue;
    }

    const tasks = iconLinks.map((iconPageUrl) =>
      limit(async () => {
        try {
          const result = await fetchSvgFromIconPage(iconPageUrl);
          if (result) {
            downloaded.push(result);
            console.log(`  → downloaded ${path.relative(outDir, result.outPath)}`);
          }
          await delay(opts.delayMs || 200);
        } catch (err) {
          errors.push({ type: "icon", iconPageUrl, error: err.message });
          console.warn(`  ✖ failed to download icon from ${iconPageUrl}: ${err.message}`);
        }
      })
    );

    await Promise.all(tasks);
  }

  return { collection, downloaded, errors };
}

async function main() {
  const outDir = process.argv.includes("--out")
    ? path.resolve(process.cwd(), process.argv[process.argv.indexOf("--out") + 1])
    : DEFAULT_OUTPUT_DIR;

  ensureDir(outDir);

  const results = [];
  for (const collection of DEFAULT_COLLECTIONS) {
    try {
      const result = await scrapeCollection(collection, outDir, {
        concurrency: 2,
        delayMs: 300,
      });
      results.push(result);
    } catch (err) {
      console.error(`Error scraping collection '${collection.name}': ${err.message}`);
      results.push({ collection, error: err.message });
    }
  }

  const summary = {
    timestamp: new Date().toISOString(),
    outputDir: outDir,
    collections: results.map((r) => ({
      name: r.collection.name,
      baseUrl: r.collection.baseUrl,
      downloadedCount: r.downloaded?.length ?? 0,
      errors: r.errors?.length ?? 0,
    })),
  };

  await fs.promises.writeFile(
    path.join(outDir, "scrape-summary.json"),
    JSON.stringify(summary, null, 2),
    "utf8"
  );

  console.log(`\nDone. Summary written to ${path.join(outDir, "scrape-summary.json")}`);
}

if (require.main === module) {
  main().catch((err) => {
    console.error("Fatal error:", err);
    process.exit(1);
  });
}
