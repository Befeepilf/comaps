# Play Console data-safety answers (Street Pixels public V1)

**Status:** Factual questionnaire for a human to paste into Play Console
  (App content → Data safety). Not marketing copy. Not a privacy policy
  (SP-093 residual). Not a rewrite of Play listing brand text
  (**SPD-079** / **SPD-084** residual).

**Scope:** Google Play `google` release is the V1 store gate (**SPD-079**).
Answers describe the public Android artefact: session-only location, no
purchase action (**SPD-010**), no friends surface (**SPD-085**), no
`ACCESS_BACKGROUND_LOCATION` (**SPD-082**).

**Source behaviour:** product spec §10, §25.1–§25.6, §32, §34; `android/app/src/main/AndroidManifest.xml`;
competition upload allow-list; Sentry meta-data in the same manifest.

Do **not** advertise GPX import/export as a free public feature. Public
V1 keeps Explorer Pro capabilities closed. Do **not** advertise friends.

Play’s definition of **collected** is data transmitted off the device.
On-device-only access is out of scope for collection.

---

## Overview questions

| Play Console question | Answer | Notes |
| --- | --- | --- |
| Does your app collect or share any of the required user data types? | **Yes** | Competition opt-in uploads and Sentry crash/diagnostics leave the device. Precise GPS tracks do not. |
| Is all user data collected by your app encrypted in transit? | **Yes** | HTTPS for competition, map downloads, OSM editor uploads, and Sentry. |
| Do you provide a way for users to request that their data is deleted? | **Yes** (competition profile) | Settings → delete competition profile removes the public profile and uploaded aggregates (spec §25.6). Privacy-policy URL for the form is **SP-093 residual**. |
| Is your app required to follow the Families policy? | **No** unless a later product lock says otherwise | Do not display the Families badge without a review. |
| Has your app been independently reviewed against a global security standard? | **No** | Do not display the MASA badge. |
| Does your app use the Unified Payments Interface (UPI)? | **No** | No purchase action in public builds (**SPD-010**). |

---

## Data not collected / not shared (do not select)

| Data type | Why not selected |
| --- | --- |
| Precise location | `ACCESS_FINE_LOCATION` is used on-device for recording and navigation. Raw GPS samples and recorded tracks are never uploaded (spec §3, §25.2, §34). Play: on-device processing is not collection. |
| Approximate location **except competition opt-in** | Without competition consent, area/city aggregates are not transmitted. |
| Financial info / purchase history | No Play Billing, no SKU, no restore, no buy button (**SPD-010**). |
| Advertising ID / ads data | No ads SDK. `AD_ID` is not in the manifest. Data is **not used for advertising or marketing**. |
| Contacts | Friends are hidden in public V1 (**SPD-085**). No add-friend intent-filters. |
| Files and docs as a cloud/GPX feature | Public V1 does not ship ungated GPX. KML/KMZ bookmark import is processed on-device. Do not claim GPX as a free public feature. |
| Photos / videos / screenshots | Sentry `attach-screenshot` and `attach-view-hierarchy` are **false**. |
| Web browsing history | Not collected. |
| Health and fitness | Not collected. |

**Sale of data:** the app does **not** sell user data.

**Background location (Play Console extra declaration):** do **not** declare
background location. `ACCESS_BACKGROUND_LOCATION` is absent (**SPD-082**).
Recording uses a location foreground service (`TrackRecordingService`,
`NavigationService`) while a session or navigation is active.

---

## Data types to select

### Location → Approximate location

| Field | Answer |
| --- | --- |
| Collected, shared, or both? | **Collected.** Treat as **shared** only in the product sense that opted-in area/city aggregates become public rankings on first-party servers (spec §25.5). Other users are not a third-party SDK. If Play’s “shared” means third-party organisations, select **Collected only** and keep the honesty sentence in the privacy policy (SP-093). Recommended paste: **Collected**; not sold; not used for ads. |
| Processed ephemerally? | **No** (queued aggregates persist until upload and are stored for rankings). |
| Required or optional? | **Optional.** Users explore without joining rankings. Collection starts only after competition consent. |
| Purposes | **App functionality** (opt-in local rankings). Not analytics. Not advertising. |
| What is transmitted | Spec §25.2 only: pseudonymous profile id, public nickname, area id, aggregate ownership score, live coverage %, eligibility, weekly new-live-pixel count **by city**, map-data version, score version, last aggregate update time. **Not** raw GPS, tracks, exact location, home/work, per-pixel timestamps, or live movement. |

Paste line: *Approximate location is collected only when the user opts into competition, as area-level and city-level aggregates. Precise GPS is not uploaded. Location is not sold and is not used for ads.*

### Personal info → Name

| Field | Answer |
| --- | --- |
| Collected, shared, or both? | **Collected** (public nickname when the user claims one for competition). |
| Required or optional? | **Optional** (competition opt-in). |
| Purposes | **App functionality** and **Account management**. |
| Notes | Nickname is shown on first-party rankings. Not a legal name. |

### Personal info → User IDs

| Field | Answer |
| --- | --- |
| Collected, shared, or both? | **Collected** (pseudonymous competition-profile identifier after opt-in). |
| Required or optional? | **Optional**. |
| Purposes | **App functionality** and **Account management**. |

### App info and performance → Crash logs

| Field | Answer |
| --- | --- |
| Collected, shared, or both? | **Collected** via Sentry as a **service provider** (not third-party “sharing” under Play’s service-provider exception). |
| Required or optional? | **Required** for crash reporting in the shipped build (no in-app opt-out). |
| Purposes | **Analytics** (diagnose crashes). |
| Notes | `io.sentry.send-default-pii` is false. Application logs are disabled. |

### App info and performance → Diagnostics

| Field | Answer |
| --- | --- |
| Collected, shared, or both? | **Collected** via Sentry (traces / profiling at 10% sample). Service provider. |
| Required or optional? | **Required** in the shipped build. |
| Purposes | **Analytics**. |
| Notes | Coupled to sampled traces; profiling does not start on app start. |

### Device or other IDs

| Field | Answer |
| --- | --- |
| Collected, shared, or both? | **Collected** if the Sentry Android SDK attaches an installation/device identifier (service provider). Not an advertising ID. |
| Required or optional? | **Required** for crash grouping in the shipped build. |
| Purposes | **Analytics**. |

### App activity → App interactions

| Field | Answer |
| --- | --- |
| Collected, shared, or both? | **Collected** only as Sentry user-interaction breadcrumbs (`io.sentry.traces.user-interaction.enable` is true). Service provider. |
| Required or optional? | **Required** in the shipped build. |
| Purposes | **Analytics**. |
| Notes | Product analytics counters stay **local** (SPD-081). They are not this Sentry path and contain no location values. |

---

## Security practices (paste)

- Data encrypted in transit: **Yes**.
- Users can request deletion: **Yes** for competition profile and uploaded aggregates from settings.
- Data is **not sold**.
- Data is **not used for advertising or marketing**.
- Independent security review: **No**.

---

## Explicit non-claims

- Do not list GPX import/export as collected files, a cloud backup, or a free public feature.
- Do not list friends, contacts, or social graph.
- Do not list precise location as shared.
- Do not declare `ACCESS_BACKGROUND_LOCATION` or Play background-location use.
- Do not paste upstream CoMaps listing sentences that claim the app “does not track” or “does not collect personal information” without the competition-opt-in exception (listing brand rewrite is residual; this questionnaire must stay accurate).

Privacy-policy and terms URLs remain **SP-093 / SPD-080 residual**. The form still needs a policy URL before Publish; that URL is not invented here.
