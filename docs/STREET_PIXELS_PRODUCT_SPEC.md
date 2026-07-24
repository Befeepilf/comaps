# Street Pixels

## V1 Product Specification

**Document status:** Product definition complete  
**Working product name:** Street Pixels  
**Base application:** CoMaps fork  
**Primary platform (public V1):** Android  
**Post-V1 platform:** iOS (deferred until after Android V1)  
**Product model:** Offline-first exploration tool with optional local competition  
**Geographic coverage:** Wherever compatible CoMaps map data is available  
**Primary audience:** Map enthusiasts, cyclists, walkers, urban explorers, completionists, and curious commuters

---

# 1\. Product summary

Street Pixels is an offline-first map application that represents publicly accessible outdoor streets and paths as collectible map pixels.

Unexplored street pixels appear red. During an active exploration session, pixels within the user’s exploration radius turn green as the user physically travels through the world.

The core product is a personal exploration tool:

Open the map, record an exploration session, see where you have and have not been, and gradually uncover the world around you.

Personal exploration is intended to work worldwide wherever compatible CoMaps map data exists. Administrative-area progress and competition use deterministic exploration areas within recognized settlements.

An optional game layer allows users to compete for control of real neighborhoods. Each neighborhood can have a current boss, determined by how much of it a user has explored through validated live sessions and how recently those pixels were visited.

The application remains useful without registration, competition, social interaction, or payment.

Public V1 targets Android. The shared architecture should remain suitable for later iOS support.

---

# 2\. Problem

People who enjoy exploring cities currently lack a simple tool that answers:

* Which streets and paths have I already explored?

* Which parts of this neighborhood are still unknown to me?

* How much of a meaningful local area have I covered?

* Can a route take me somewhere new rather than merely somewhere quickly?

* How does my local exploration compare with other explorers?

Most fitness and navigation applications emphasize speed, distance, exercise, or efficient routing. They do not treat geographical exploration itself as the primary activity.

Street Pixels changes the optimization target:

Instead of optimizing only for efficiency, the map can optimize for curiosity.

---

# 3\. Product principles

## 3.1 Tool first

The default experience is a calm and practical exploration map.

Competition must never be required to use the core application.

## 3.2 Private by default

Personal street-pixel history and raw GPS tracks remain on the device.

No personal competition statistics are uploaded until the user explicitly enables competition and consents to sharing aggregated area-level statistics.

## 3.3 Explicit recording

Street Pixels does not continuously track the user simply because the application is installed.

Exploration is collected only during an explicit recording session started by the user.

A recording session may continue while:

* The application is in the background.

* The screen is off.

* The user is using another application.

## 3.4 Immediate comprehension

A new user should understand the basic mechanic by seeing red pixels turn green as they move.

The application should not require a lengthy tutorial.

## 3.5 Meaningful geographical units

Progress and ownership use deterministic exploration areas derived from a versioned, country-specific administrative-area configuration within each recognized settlement.

The application does not invent arbitrary square-grid neighborhoods or infer area boundaries from point-only place tags.

## 3.6 Permanent personal progress

Once a street pixel has been explored, it remains explored on the user’s personal map.

Competitive recency may decay, but personal exploration completion does not.

## 3.7 Optional fun

Milestones, animations, haptics, sharing, ownership, and rankings enrich the experience without turning the application into a noisy mobile game.

## 3.8 Offline reliability

Core map viewing, recording, street-pixel collection, progress calculation, track storage, and supported routing continue to work without an internet connection.

---

# 4\. Target users

## 4.1 Primary user: the dedicated explorer

This user deliberately tries to cover streets and neighborhoods, notices gaps on maps, may import historical tracks when Explorer Pro is enabled, and may use the application for months or years.

Typical goals:

* Track comprehensive exploration.

* Complete neighborhoods.

* Find unvisited streets.

* Import historical journeys when Explorer Pro is enabled in the build.

* Export recorded tracks when Explorer Pro is enabled in the build.

* Maintain a reliable personal exploration archive.

This is the initial target audience because the product originates from a real need within this group.

## 4.2 Secondary user: the curious cyclist or walker

This user does not necessarily aim for complete coverage but enjoys taking new routes and seeing visible progress.

Typical goals:

* Make routine journeys more interesting.

* Find a different route to work.

* Discover unfamiliar nearby streets.

* See visible progress after a walk or ride.

## 4.3 Secondary user: the competitive local

This user is motivated by neighborhood ownership, rankings, and overtaking others.

Typical goals:

* Become boss of a home neighborhood.

* Defend local control.

* Move up the weekly city ranking.

* Share completed neighborhoods.

---

# 5\. Public Android V1 goals

Public Android V1 must allow a user to:

1. Open the application and see unexplored street pixels.

2. Start and stop an explicit exploration-recording session.

3. Continue recording with the application in the background and the screen off.

4. Collect pixels through valid real-world movement.

5. Track permanent exploration progress by exploration area.

6. See progress update frequently enough to feel meaningful.

7. Receive restrained milestone feedback.

8. Route through unexplored streets.

9. Opt into local competition without creating a conventional email-and-password account.

10. Compete to become boss of an exploration area.

11. See relevant exploration-area and city rankings.

12. Share a completed exploration area.

13. Use the core personal exploration functions offline.

14. Share a 100% area-completion card without exposing private location data.

Public Android V1 does not require Explorer Pro purchasing, Google Play Billing integration, or purchase restoration. Explorer Pro GPX tooling may exist behind feature flags in internal or beta builds but is not required for public launch.

---

# 6\. Public Android V1 non-goals

The following are explicitly excluded from public Android V1:

* User-created map markers.

* Easter eggs or hidden collectibles.

* User-submitted discoveries.

* Comments, messages, or social feeds.

* Friend requests or family groups.

* Global public leaderboards.

* Country-level public leaderboards.

* Country exploration percentages.

* World exploration percentages.

* Complex achievement trees.

* Achievement-history screens.

* Daily quests.

* Sponsored locations.

* Advertising.

* Live user locations.

* Exact-location sharing.

* Nearby-user discovery.

* Showing that another player is physically nearby.

* Raw GPS-track uploads for competition.

* Automatic background tracking outside a user-started session.

* Sophisticated server-side anti-cheat.

* Perfect consistency between users running different offline map versions.

* Aggressive retention notifications.

* Generated exploration missions.

* Scenic-place recommendations.

* iOS public release, StoreKit integration, and iOS-specific launch gates.

* Live Explorer Pro purchasing, Google Play Billing integration, and purchase restoration in public builds.

* City allowlists, pilot-only runtime behavior, or city-specific feature restrictions.

These ideas may be reconsidered after V1, but they must not delay the initial Android release.

---

# 7\. Core terminology

## Street pixel

A fixed HEALPix cell associated with at least one eligible street or path.

Eligible street and path geometry is sampled approximately every 10 metres. Each sample point is assigned to its nearest HEALPix cell. Duplicate cells are removed.

## Unexplored pixel

A valid street pixel the user has never collected.

Displayed in red.

## Explored pixel

A valid street pixel collected through either:

* A validated live recording session.

* An imported GPX track.

Displayed in green.

## Live exploration pixel

A pixel collected during a validated Street Pixels recording session.

Only live exploration pixels can affect competitive recency, neighborhood ownership, boss eligibility, and weekly rankings.

## Focused area

The administrative exploration area whose progress is currently shown in the primary progress badge.

Within a recognized settlement, this is normally the current exploration area: the most local valid administrative subdivision, place boundary, or settlement fallback configured for that country.

Where no suitable subdivision or place boundary exists, the containing city, town, or village becomes the focused area.

## Area completion

The percentage of valid street pixels in an area that the user has explored.

\[  \=  {}  \]

Both validated live pixels and imported GPX pixels affect personal completion.

## Pixel recency

A competitive value representing how recently the user visited a pixel during a validated live recording session.

Imported GPX tracks do not create or refresh pixel recency.

## Neighborhood boss

The eligible competition participant with the highest current ownership score in an area.

## Competition mode

The opt-in game layer showing ownership, rankings, relative competitive progress, and weekly city performance.

## Explorer Pro

The paid advanced-tool upgrade for GPX import, export, and related track management.

Explorer Pro capabilities are gated by entitlement. In public Android V1, Pro features may be disabled globally by feature flag even though the entitlement architecture remains in place for later activation.

Imported GPX data is strictly excluded from competition regardless of feature-flag or entitlement state.

---

# 8\. Geographic model

## 8.1 Core map coverage

Street-pixel exploration is available wherever compatible CoMaps map data exists.

Personal exploration is intended to work worldwide where compatible map data exists. Administrative-area progress and competition work wherever suitable generated polygons are available for the installed map-data version.

## 8.2 Settlement detection

Where an OSM city, town, village, or equivalent municipal settlement boundary is available, Street Pixels associates local exploration areas with that settlement.

Settlement boundaries provide the geographic context for administrative subdivisions, place-boundary fallbacks, and settlement-level fallback areas.

## 8.3 Exploration-area selection policy

Exploration areas are selected using a versioned country-specific administrative-area configuration. The configuration is based initially on the OpenStreetMap administrative-level reference:

`https://wiki.openstreetmap.org/wiki/Tag:boundary%3Dadministrative#Table_:_Admin_level_for_all_countries`

One `admin_level` value is not treated as semantically identical worldwide. Country configurations determine which administrative levels are valid and in what priority order.

The intended area-selection hierarchy is:

1. Determine the containing recognized settlement: city, town, village, or equivalent municipal settlement boundary.

2. Within that settlement, prefer the most local valid administrative subdivision configured for that country.

3. Country configurations should generally prefer the most specific useful level, commonly `admin_level=11`, then `admin_level=10`, followed by any other country-specific local levels when the country reference indicates they are more appropriate.

4. If no suitable administrative subdivision exists, try polygonal place boundaries representing neighborhood, quarter, or suburb.

5. Only actual closed polygon boundaries may be used. Do not invent polygons around `place=*` nodes.

6. If no valid subdivision or place polygon exists, fall back to the containing city, town, or village.

7. Outside a recognized settlement:

   * Personal street-pixel exploration still works.

   * Routing still works where supported.

   * Area completion and competition are unavailable unless a suitable area is defined later.

The exact per-country configuration and tagging rules belong in a versioned map-data policy or technical specification. The product spec does not reproduce the full country table.

## 8.4 Candidate-area suitability

A candidate exploration area must be:

* A closed valid polygon.

* Consistently identifiable.

* Normally named.

* Associated with the containing settlement.

* Meaningfully smaller than the containing settlement.

* Containing a meaningful street-pixel set.

* An ordinary geographic subdivision rather than an electoral, ecclesiastical, statistical, emergency, or operational boundary.

## 8.5 Settlement fallback

Where a settlement has no suitable administrative subdivision or place-boundary polygon:

* The entire settlement becomes one exploration area.

* Settlement completion and local-area completion are the same value.

* Settlement-wide ownership may be used for competition.

## 8.6 Outside recognized settlements

Outside a recognized settlement boundary:

* Street-pixel exploration still works.

* Pixels remain permanently red or green according to personal exploration.

* Recorded tracks remain available.

* Routing remains available where supported.

* Area completion and local competition are unavailable unless a suitable exploration area is later defined.

V1 does not create arbitrary grid areas as a fallback.

## 8.7 Launch campaigns and worldwide availability

Street Pixels remains globally available wherever compatible CoMaps map data exists.

City-focused launch campaigns may be used for marketing and community-building, but they do not restrict product availability. A featured launch city or city challenge is a marketing concept, not a technical availability rule.

Competition may naturally be sparse outside promoted cities, but the application must not imply that unsupported marketing regions are technically unavailable.

V1 does not use city allowlists, pilot-only runtime behavior, or city-specific feature restrictions.

## 8.8 Deterministic area assignment

Every valid street pixel belongs to no more than one selected exploration area.

Area assignment must be deterministic for the same generated map-data and policy versions.

Assignment rules:

* Prefer the highest-priority configured administrative level for the country.

* Where multiple valid polygons at the same priority contain the pixel, select the smallest polygon.

* Resolve any remaining tie using a stable identifier such as the OSM relation ID.

The same generated map-data version must always assign the same pixel to the same area.

---

# 9\. Main application structure

Public Android V1 has two conceptual layers within one application. The shared architecture should remain suitable for later iOS support.

## 9.1 Explore layer

This is the default and primary experience.

It contains:

* Base map.

* Red and green street pixels.

* Current position.

* Recording controls.

* Current recording status.

* Area exploration percentage.

* Route planning.

* Exploration milestones.

* Personal completion states.

* Locally stored recording history.

No account or competition consent is required.

## 9.2 Competition layer

This is optional and must be enabled deliberately.

It contains:

* Area boss information.

* Ownership score.

* Local ranking.

* Overtaking hints.

* Weekly city leaderboard.

* Public nickname.

* Aggregated statistic uploads.

The two layers share the same local exploration data. They are not separate applications or separate profiles.

---

# 10\. First-launch user journey

## Step 1: Application opens

The user sees a map.

Where location access has not yet been granted, the application may show the last selected map area or a neutral starting view.

Nearby street pixels appear red because no exploration history exists yet.

The map—not a login form, dashboard, or game screen—is the first meaningful interface.

## Step 2: Initial explanation

A small onboarding card explains:

**Heading:** Explore the streets around you

**Body:** Start a recording session and nearby streets will turn from red to green as you explore. Your personal routes and exploration history stay on this device unless you later choose to join local rankings.

Primary action:

**Start exploring**

## Step 3: Location permission

When the user taps **Start exploring**, the application requests the location permissions required to record an active session.

The permission explanation must connect the request directly to the feature.

It must not bundle competition consent into the location-permission flow.

## Step 4: Background-recording explanation

Before requesting any background-location capability required by the operating system, the application explains:

Recording can continue while the screen is off or Street Pixels is in the background. Tracking stops when you end the session.

The application must not claim to track continuously outside active sessions.

## Step 5: Recording begins

The recording state becomes clearly visible.

The recording control supports:

* Pause.

* Resume.

* Finish.

A persistent operating-system indicator or notification is shown where required by the platform.

## Step 6: First goal appears

A small, non-blocking milestone badge appears near the top of the map:

**Explore your first 100 m**

It includes an incomplete progress indicator.

This is contextual onboarding, not part of a large achievement system.

## Step 7: User begins moving

Accepted GPS updates collect valid street pixels within the exploration radius.

Pixels turn from red to green as soon as they are collected.

This transformation is the application’s primary “aha” moment.

## Step 8: First haptic feedback

When at least one new pixel is collected during an accepted foreground update, the device produces one subtle haptic pulse.

The application does not vibrate once for every individual pixel.

No exploration haptics are produced while the screen is off or the application is backgrounded.

## Step 9: First goal completes

After the equivalent of approximately 100 metres of new live street pixels has been collected, the first-goal badge completes with a small animation.

The user remains on the map.

## Step 10: Competition is hinted at

After the user has collected at least 30 new live pixels, approximately 300 metres, the application may show one temporary, non-blocking competition hint.

Possible states include:

**Where another participant is ahead**

Another explorer is ahead of you in Kallio.

**Where the user is approaching leadership eligibility**

You’re getting close to qualifying for the lead in Kallio.

**Where the user already leads**

You currently lead Kallio.

**Where comparison data exists**

See how your Kallio exploration compares.

The hint must not say that another user is physically nearby or currently active.

## Step 11: User opens the competition hint

Tapping the hint opens the competition introduction.

The application explains what will be shared and what remains private.

## Step 12: User chooses

The user may:

* Join competition.

* Dismiss the prompt.

* Continue using the complete personal exploration tool privately.

Dismissal must not weaken the free experience.

Competition may be enabled later through map controls or settings.

---

# 11\. Recording sessions

## 11.1 Explicit start

Street pixels are collected automatically only while a user-started recording session is active.

Opening the application alone does not begin recording.

## 11.2 Background continuation

An active recording session continues when:

* The application moves to the background.

* The screen is turned off.

* The user opens another application.

The session ends only when:

* The user finishes it.

* The user explicitly discards it.

* The operating system terminates tracking and the application cannot recover.

## 11.3 Pause

While paused:

* Location data is not used to collect pixels.

* No competitive recency is refreshed.

* No track segment is created across the paused period.

Resuming begins from the new valid location without interpolating across the pause.

## 11.4 Session completion

When the user finishes a session:

* The recorded track is stored locally.

* New pixels remain permanently explored.

* Area statistics are recalculated.

* Eligible competition aggregates are queued for delayed upload.

* The user may inspect or delete the recorded track.

## 11.5 Interrupted sessions

If the operating system interrupts background tracking:

* No line is interpolated across the missing period.

* Recording may resume from the next accepted sample.

* The application informs the user that part of the session may be missing.

---

# 12\. Map and progress experience

## 12.1 Street-level zoom

At close zoom levels:

* Individual street pixels are visible.

* Red means unexplored.

* Green means explored.

* The focused area’s name and percentage appear in the primary badge.

* The map remains the dominant visual element.

Percentage labels are not placed across every neighborhood by default.

## 12.2 Neighborhood-level zoom

At neighborhood scale:

* Street pixels remain visible where practical.

* Area boundaries may appear subtly.

* Completed areas receive a distinct completion outline or check indicator.

* The primary progress badge remains interactive.

Tapping an area focuses it and reveals its exact personal completion percentage.

## 12.3 City-level zoom

At city scale:

* Individual pixels may fade or aggregate to preserve readability.

* Administrative areas may be shaded according to completion.

* Exact percentages are shown only after selection.

* The primary summary badge may show overall city completion.

City completion is secondary context. Neighborhood-level completion remains the main everyday progress metric where neighborhoods exist.

## 12.4 Country and world zoom

V1 does not calculate or display country or world exploration percentages.

At wider zoom levels, the user may still browse the base map, but large-scale percentage statistics are reserved for a later version.

## 12.5 Focus behavior

The focused area follows these rules:

1. During active recording or after recentering, focus follows the user’s current area.

2. Panning away may focus the area beneath the map centre.

3. Tapping an area explicitly focuses it.

4. Recentring returns focus to the user’s current position.

5. Zooming to city scale changes the summary badge to city completion.

Area-name transitions must make numerical changes understandable.

---

# 13\. Eligible street and path data

## 13.1 Included routes

V1 includes outdoor, land-based OSM ways that are legally or ordinarily accessible on foot or by bicycle.

This includes:

* Public streets.

* Public roads.

* Cycleways.

* Footways.

* Shared pedestrian and bicycle routes.

* Public service roads.

* Outdoor pedestrian streets.

* Steps.

* Informal outdoor paths.

* Tracks.

* Bridges carrying an otherwise eligible land route.

* Motorways or controlled-access roads where bicycle access is explicitly permitted.

Informal paths are included unless access tags clearly prohibit use.

## 13.2 Excluded routes

V1 excludes:

* access=private.

* access=no.

* Routes explicitly inaccessible to both pedestrians and cyclists.

* Indoor corridors and passages.

* Underground routes.

* Subway or metro passages.

* Ferry routes.

* Other waterborne links.

* Aerial transport links.

* Proposed routes.

* Construction-only routes.

* Emergency-only routes.

* Clearly restricted operational or service infrastructure.

## 13.3 Ambiguous access

Where access information is missing, Street Pixels follows the ordinary access assumptions used by the underlying CoMaps pedestrian and bicycle data, except for the explicit exclusions above.

## 13.4 Data-policy appendix

The precise OSM-tag inclusion and exclusion rules must be maintained as a versioned map-data policy accompanying the implementation.

The product rules above remain the source of truth for that policy.

---

# 14\. Street-pixel generation

## 14.1 Fixed HEALPix grid

Street Pixels uses a fixed HEALPix grid.

The grid scale must be sufficiently fine that eligible streets and paths map accurately without producing visibly misplaced pixels.

## 14.2 Path sampling

Eligible street and path geometry is sampled approximately every 10 metres.

For each sample point:

1. Find the nearest HEALPix cell.

2. Add that cell to the valid street-pixel set.

3. Remove duplicates.

## 14.3 Stability

A street pixel is identified by its HEALPix cell identifier and the map-data version.

The generation process must be deterministic for the same source data and configuration.

---

# 15\. Exploration collection rules

## 15.1 Exploration radius

The V1 exploration radius is fixed at **25 metres**.

Any valid street pixel within 25 metres of an accepted live location or accepted interpolated segment may be collected.

The radius represents practical visual and spatial exploration rather than exact foot placement. A person can experience nearby paths and surroundings without standing precisely on every mapped centreline.

The radius is not user-configurable in V1.

## 15.2 Permanent exploration state

When a pixel is collected for the first time:

* It turns green.

* It contributes to personal completion.

* Its first-explored source is stored.

* If collected live, its competitive recency timestamp is created.

On later validated live visits:

* It remains green.

* Personal completion does not increase again.

* Its competitive recency timestamp is refreshed.

## 15.3 Imported-pixel behavior

When a pixel is collected through GPX import:

* It turns green.

* It contributes to personal completion.

* It is marked as imported or historical.

* It does not receive a competitive recency timestamp.

* It does not contribute to weekly competition.

* It does not create or refresh neighborhood ownership.

## 15.4 Area assignment

Every valid street pixel within a supported settlement belongs to one selected exploration area according to the versioned administrative-area policy.

When the user collects pixels near an area boundary, each pixel contributes to the area that contains that pixel.

There is no rule granting the same pixel to two areas.

## 15.5 Movement types

V1 treats valid walking and cycling movement equally.

A valid pass collects pixels regardless of:

* Travel direction.

* Time spent on the street.

* Whether the user is walking or cycling.

* Whether the street has previously been explored from another direction.

---

# 16\. GPS validation and interpolation

The application must never draw a continuous explored line across a long loss of location data.

## 16.1 Live samples only

Competitive exploration is generated only from live location samples recorded by Street Pixels during an active session.

Imported GPX data is processed separately as personal historical data.

## 16.2 Accepted sample defaults

A live location sample is accepted when:

* Reported horizontal accuracy is 25 metres or better.

* The sample is not stale.

* The operating system has not marked it invalid.

* The implied speed from the previous accepted sample is no more than 50 km/h.

* The movement does not represent an implausible teleport.

These are V1 launch defaults and may be adjusted through field testing.

## 16.3 Interpolation

Street Pixels interpolates movement between two consecutive accepted samples when all of the following are true:

* Both samples pass validation.

* The time gap is no more than 30 seconds.

* The distance between samples is no more than 200 metres.

* The implied speed is no more than 50 km/h.

* Neither sample follows a pause, interrupted session, or rejected jump.

Pixels within the 25-metre radius of the valid interpolated segment may be collected.

## 16.4 Rejected gaps and jumps

No connecting line is created when:

* The time gap exceeds 30 seconds.

* The distance exceeds 200 metres.

* The implied speed exceeds 50 km/h.

* Either sample has unacceptable accuracy.

* Recording was paused.

* Background tracking was interrupted.

* The location jumps after a signal loss.

The new accepted point becomes the starting point for future interpolation.

Pixels may be collected around the new point itself, but never along the invalid gap.

## 16.5 Poor GPS state

When GPS quality deteriorates:

* Existing exploration remains safe.

* The application may show **Waiting for accurate location**.

* Exploration resumes from the next accepted point.

* Missing streets are not automatically filled.

---

# 17\. Routing

V1 retains exploration-aware routing as a major product feature.

## 17.1 Standard routing

The user can create ordinary walking or cycling routes using the underlying CoMaps routing capabilities.

## 17.2 Prefer unexplored streets

A route option labelled **Prefer unexplored streets** increases the preference for graph edges containing a greater share of unvisited pixels.

The route should remain reasonably practical while introducing new streets where possible.

This mode is free.

## 17.3 Avoid explored streets

An advanced option labelled **Avoid explored streets** attempts to avoid graph edges containing explored pixels.

The interface must warn:

This can produce very long routes or no available route.

Where a strictly unexplored route is impossible, the application must explicitly offer:

* Allow the minimum necessary explored connection.

* Return to normal routing.

It must not silently abandon the selected rule.

## 17.4 Deferred routing modes

The following are not required in V1:

* Maximize new pixels within a time limit.

* Generate completion loops.

* Scenic plus unexplored routing.

* Multiplayer routes.

* Automatic neighborhood-completion routes.

---

# 18\. Progress milestones

V1 uses sparse contextual milestones rather than a broad achievement system.

## 18.1 Area milestones

Each area has celebrations at:

* 25% explored.

* 50% explored.

* 100% explored.

## 18.2 25% milestone

The application displays a small, non-blocking acknowledgment:

25% of Kallio explored

A brief animation may appear around the progress badge.

## 18.3 50% milestone

The application displays a stronger acknowledgment:

Half of Kallio explored

A light haptic and a slightly more visible animation may be used.

## 18.4 100% milestone

The application presents the primary completion celebration:

* Area outline glows or fills.

* Completed state becomes visually marked.

* A stronger haptic may play.

* A completion card appears.

* A share action is offered.

* Competition status is included where competition is enabled.

Suggested primary text:

Kallio fully explored

The celebration must not interrupt active routing or require immediate interaction.

## 18.5 No achievement history

V1 does not provide:

* An achievement list.

* A trophy cabinet.

* A milestone-history screen.

* Achievement points.

The application may store the original 100% completion date locally so it can be included in a share card or used after map-data changes.

## 18.6 Completed visual state

Completed areas remain active rather than becoming visually irrelevant.

At wider scales they receive:

* Green completion outline.

* Subtle check indicator.

* Restrained completed fill or pattern.

At close zoom, the ordinary street-pixel map remains visible.

---

# 19\. Shareable completion cards

After reaching 100%, the user may create a shareable image.

Sharing is always optional.

## 19.1 Card contents

The card includes:

* Area name.

* “100% explored.”

* Stylized map or boundary outline.

* Optional display name.

* Optional completion date.

* Subtle Street Pixels branding.

It must not include:

* Raw GPS route.

* Home location.

* Live location.

* Individual timestamps.

* Other users’ personal information.

## 19.2 Anonymous sharing

Users who have not joined competition can share a first-person card:

I explored 100% of Kallio

No account or public nickname is required.

## 19.3 Share flow

The completion card contains a **Share** button.

The application does not automatically open the operating-system share sheet.

---

# 20\. Competition opt-in

## 20.1 Default state

Competition is disabled by default.

Personal pixels, recording, routing, milestones, track storage, and completion statistics work without competition.

## 20.2 Opt-in explanation

The competition introduction states:

Join local exploration rankings and compete for neighborhood control.

It must clearly explain:

* Aggregated area statistics will be uploaded.

* A public nickname will appear where rankings have enough participants.

* Exact routes are not uploaded.

* Raw GPS positions are not uploaded.

* Live location is not shared.

* Nearby-user discovery does not exist in V1.

* Participation can be disabled later.

## 20.3 Installation identity

Before opt-in, the application creates a random pseudonymous installation identifier stored on the device.

This identifier:

* Is not a hardware serial number.

* Is not shown publicly.

* Is not derived from advertising identifiers.

* Does not require email registration.

## 20.4 Public nickname

When joining competition:

* The application suggests an automatically generated nickname.

* The user may edit it.

* No email address or password is required.

* Nicknames do not need to be globally unique.

* An internal profile identifier distinguishes identical names.

V1 does not guarantee cross-device profile recovery.

Reinstallation or device loss may disconnect the user from the original competition profile.

## 20.5 Consent record

The user must actively confirm competition participation.

The consent record includes:

* Competition enabled.

* Aggregated-stat sharing enabled.

* Current privacy-policy version.

* Consent timestamp.

Competition consent is separate from location permission.

## 20.6 Leaving competition

The user may disable competition from settings.

The user is offered a choice to:

* Stop future uploads while retaining existing public statistics.

* Delete the public competition profile and uploaded statistics.

Local personal exploration remains intact.

---

# 21\. Nickname rules and moderation

## 21.1 Format

Public nicknames must:

* Contain between 3 and 24 visible characters.

* Exclude control characters.

* Exclude line breaks.

* Exclude URLs.

* Exclude email addresses.

* Exclude phone numbers.

* Avoid excessive invisible or combining characters.

* Be normalized before validation.

Unicode letters and ordinary non-Latin writing systems are permitted.

Spaces, numbers, underscores, hyphens, and limited ordinary punctuation are permitted.

## 21.2 Prohibited content

Nicknames may not contain:

* Hate speech.

* Threats.

* Targeted harassment.

* Sexual content involving minors.

* Explicitly abusive slurs.

* Impersonation intended to deceive.

* Commercial spam.

* Contact or solicitation information.

* References presented as official Street Pixels staff identities without authorization.

## 21.3 Filtering and enforcement

V1 includes:

* Basic profanity and abuse filtering.

* A nickname-report action.

* Administrative reset or removal.

* Temporary or permanent competition suspension for repeated abuse.

## 21.4 Renaming

Users may rename themselves no more than once every seven days.

Administrative resets do not count against the rename interval.

## 21.5 Limited profile surface

V1 has no:

* Biography.

* Profile image.

* External link.

* Social handle.

* Free-form status.

* Direct messaging.

This keeps moderation scope intentionally small.

---

# 22\. Neighborhood ownership

## 22.1 Principle

Ownership rewards both coverage and recent activity.

A user who explored an area extensively long ago should not necessarily remain boss forever, while a user who recently travelled one street should not instantly gain control.

## 22.2 Competitive pixel set

Only pixels collected during validated live Street Pixels sessions contribute to:

* Recency.

* Ownership score.

* Boss qualification.

* Competitive area coverage.

Imported GPX data does not affect these values.

## 22.3 Pixel recency weight

Each live-explored pixel has a recency weight determined by its most recent validated live visit.

V1 uses an approximately 30-day half-life:

* Just visited: weight close to 1.0.

* 30 days since visit: approximately 0.5.

* 60 days: approximately 0.25.

* 90 days: approximately 0.125.

* Older visits continue approaching zero.

Revisiting the pixel during a valid live session restores its weight to approximately 1.0.

## 22.4 Ownership score

For each participant and area:

\[  \=  {}  \]

This combines:

* Live competitive coverage.

* Percentage of the area explored live.

* Recency of those pixels.

Pixel count and area percentage are not multiplied separately because that would double-count coverage.

## 22.5 Boss eligibility

A participant becomes eligible to be boss only when all of the following are true:

* At least 2% of the area’s valid pixels have been collected through validated live sessions.

* At least 50 unique live pixels have been collected.

* Current recency-weighted ownership score is at least 0.5%.

Where an area contains fewer than 50 total pixels, the percentage and current-score requirements apply without the 50-pixel minimum.

## 22.6 Selecting the boss

The eligible participant with the highest current ownership score is the boss.

## 22.7 Unclaimed areas

An area is unclaimed when:

* No participant meets the eligibility requirements.

* The previous boss’s recency-weighted score has decayed below the minimum.

* All eligible participants have withdrawn from competition.

An area can therefore lose its boss without another participant immediately taking over.

## 22.8 Server-side score decay

The backend stores:

* Last accepted aggregate ownership score.

* Time of that score.

* Recency-decay version.

Between uploads, the backend applies the same exponential decay to the stored aggregate score.

This allows inactive bosses to lose eligibility even when their application is not opened.

A new client upload replaces the decayed estimate with the newly calculated aggregate score.

## 22.9 Contested state

An area is labelled **contested** when:

\[  %  \]

The leader remains boss until overtaken.

## 22.10 Completion does not guarantee ownership

Reaching 100% personal completion does not automatically make the user boss.

Reasons include:

* Some pixels may come from GPX import.

* Older live pixels may have weak recency.

* Another participant may have stronger current coverage.

Possible completion messages:

**User becomes boss**

Kallio fully explored — and you now lead the area.

**User does not lead**

Kallio fully explored. Revisiting older streets can strengthen your position.

The application must not imply that personal completion was invalid.

---

# 23\. Competition interface

## 23.1 Mode control

After opt-in, a compact map control becomes available:

* **Explore**

* **Competition**

Explore remains the default mode unless the user deliberately switches.

## 23.2 Competition map

Competition mode may show:

* Area boundaries.

* Current boss.

* Contested state.

* User ownership score.

* Gap to the next relevant participant.

* Personal completion status.

It must not replace the red and green map with an unreadable territory layer.

## 23.3 Ranking snapshot

The local ranking component normally shows:

* Top three participants.

* Current user.

If the current user is already in the top three, the user is not duplicated.

## 23.4 Sparse-area privacy

Where fewer than three opted-in participants exist in an area:

* Other participants are described anonymously.

* Public nicknames are not shown in the area snapshot.

* Relative scores may still be shown.

Examples:

Another explorer is 1.8 points ahead.

You currently lead this area.

Explore 120 m more to qualify for leadership.

The application never says that another participant is currently nearby.

## 23.5 Overtaking hints

The application may occasionally show contextual hints such as:

* One more street could move you into second place.

* You’re close to taking the lead.

* Kallio is contested.

* Your lead is narrowing.

* You are close to qualifying for leadership.

Hints are:

* Non-blocking.

* Rate-limited.

* Based on delayed area-level statistics.

* Not evidence of another user’s live location.

---

# 24\. Weekly city leaderboard

V1 includes one broader public leaderboard scoped to a city.

## 24.1 Metric

The city leaderboard ranks participants by the number of newly explored unique live pixels collected within that city during the current week.

The following do not increase the weekly score:

* Revisiting previously explored pixels.

* Imported GPX tracks.

* Pixels collected outside validated live sessions.

## 24.2 Reset

The leaderboard resets weekly according to the city’s local time where available.

The application displays the remaining weekly period.

## 24.3 Scope

The leaderboard uses the recognized city boundary.

There are no global or country-wide public leaderboards in V1.

## 24.4 Presentation

The city leaderboard shows:

* Top local explorers.

* Current user’s rank.

* New live pixels collected this week.

* Time remaining in the weekly period.

It is secondary to local ownership and is accessed through competition mode rather than displayed permanently on the main map.

---

# 25\. Competition privacy and uploads

## 25.1 Local-only information

Unless competition is enabled, the following remains local:

* Explored pixel IDs.

* Raw GPS samples.

* Recorded GPS tracks.

* Pixel timestamps.

* Area percentages.

* Routing history.

* Imported GPX data.

* Recording-session history.

## 25.2 Uploaded competition aggregates

When competition is enabled, the application uploads only:

* Pseudonymous competition-profile identifier.

* Public nickname.

* Area identifier.

* Current aggregate ownership score.

* Live competitive coverage percentage.

* Eligibility state.

* Weekly new-live-pixel count by city.

* Map-data version.

* Score-calculation version.

* Last aggregate update time.

The backend does not require:

* Raw GPS points.

* Full GPS tracks.

* Exact current location.

* Home or work location.

* Per-pixel visit timestamps.

* Live movement state.

## 25.3 Delayed batching

Competition updates are not uploaded immediately after every location change.

V1 behavior:

* Changes are queued locally.

* Upload occurs no more frequently than once every 15 minutes during an active online session.

* The upload may include an additional randomized delay of up to 15 minutes.

* Final session aggregates may be queued when the session ends.

* Offline updates are uploaded after connectivity returns.

This prevents competition data from functioning as a live-location signal.

## 25.4 No location discoverability

V1 does not provide:

* A map of other users.

* Nearby-user discovery.

* Live activity indicators.

* Distance to another user.

* Notifications that another participant is physically present.

* Exact last-seen locations.

Any future exact-location or nearby-discovery feature would require a separate explicit opt-in and a new privacy review.

## 25.5 Area-level implications

Competition participation necessarily reveals that a user has explored a named administrative area.

This must be stated honestly during opt-in.

The application must say that it shares area-level exploration statistics, not that it shares no location-related information whatsoever.

## 25.6 Deletion

Users can delete their public competition profile and uploaded aggregates directly from settings.

---

# 26\. Offline behavior and synchronization

## 26.1 Offline core

Without internet access, users can:

* View downloaded maps.

* See red and green pixels.

* Start and complete recording sessions.

* Collect pixels.

* Calculate area and city progress.

* Complete milestones.

* Store tracks locally.

* Use supported offline routing.

## 26.2 Competition while offline

When competition is enabled but connectivity is unavailable:

* Personal exploration continues.

* Competitive aggregates are calculated locally.

* Updates are queued.

* The last downloaded rankings remain visible with an offline or stale-data indicator.

* New aggregates upload after connectivity returns.

## 26.3 Conflict handling

The backend accepts the newest valid aggregate statistics submitted for the relevant map-data and scoring version.

V1 trusts client-calculated aggregates.

Users running different map-data versions may temporarily have slightly inconsistent percentages.

This is an accepted V1 limitation.

---

# 27\. Map-data updates

## 27.1 Manual updates

V1 uses an explicit map-update action rather than silently replacing local map data.

Before updating, the application states:

Updated map data may add or remove streets. Your exploration percentages may change slightly.

## 27.2 Recalculation

After an update:

* Existing explored HEALPix IDs are matched against the new dataset.

* Area assignments are recalculated.

* Area denominators are recalculated.

* New valid street pixels appear red.

* Removed street pixels no longer count.

* Personal percentages are recalculated.

* Competition aggregates are recalculated locally.

## 27.3 Communicating reductions

When completion decreases because new streets were added, the application says:

New streets were added to Kallio. There is more to explore.

The application does not describe this as lost or revoked progress.

## 27.4 Previous completion

If an area previously reached 100% and later falls below 100% after a map update:

* The map displays the current percentage.

* The original completion date may remain stored locally.

* No general achievement-history screen is created.

* The interface may state that the area was previously completed.

## 27.5 Competition versioning

Competition uploads include the map-data version.

V1 accepts aggregates from supported older versions rather than requiring exact cross-user normalization.

Strict server-side map-version reconciliation is deferred.

---

# 28\. Haptics and visual feedback

## 28.1 Default

Exploration haptics are enabled by default while:

* A recording session is active.

* The application is visible in the foreground.

No exploration haptics occur while the screen is off or the application is backgrounded.

## 28.2 Collection pulse

One subtle pulse occurs when an accepted foreground update collects at least one new pixel.

The application does not produce a separate pulse for every pixel.

## 28.3 Milestone feedback

Stronger haptic patterns may be used for:

* First 100 metres.

* 50% area completion.

* 100% area completion.

* Becoming area boss.

## 28.4 Setting

A single **Exploration haptics** toggle is available.

V1 does not require multiple strength settings.

---

# 29\. Explorer Pro monetization

The free version remains a complete personal neighborhood-exploration tool.

Explorer Pro purchasing is deferred from public Android V1. The Pro feature architecture remains in the codebase behind feature flags and an entitlement abstraction suitable for activating paid Explorer Pro later.

Public Android V1 must distinguish:

* **Feature available in this build** — whether Pro capabilities are enabled by feature flag.

* **User has entitlement** — whether the user has unlocked Explorer Pro.

For public V1, Pro features may be disabled globally even if the entitlement architecture exists. The UI must not present a non-functional purchase action.

## 29.1 Free features

Free users receive:

* Live street-pixel collection.

* Explicit foreground and background recording sessions.

* Local track storage.

* Offline exploration map.

* Area and city progress.

* 25%, 50%, and 100% milestones.

* Exploration-aware routing, including **Avoid explored streets**.

* Optional local competition.

* Area ownership.

* Weekly city leaderboard.

* Completion sharing.

* Manual map updates.

## 29.2 Explorer Pro features

Explorer Pro is designed to unlock:

* GPX track import.

* GPX track export.

* Batch GPX import where supported.

* Advanced local track-management tools.

* Additional export formats where supported.

These capabilities may exist in internal or beta builds behind a feature flag. They are not required in public Android V1.

Imported GPX tracks update personal exploration only.

They do not affect competitive recency, ownership, eligibility, or weekly rankings regardless of feature-flag or entitlement state.

## 29.3 Commercial model

The intended commercial model is a one-time Explorer Pro purchase rather than a mandatory subscription, unless changed by a later product decision.

Pricing, billing integration, purchase flow, purchase restoration, and store entitlement validation are post-V1 requirements for public release.

## 29.4 Monetization principle

Paid functionality targets advanced users who need data portability and historical-track tooling.

The paywall does not block:

* Basic exploration.

* Live recording.

* Area completion.

* Local competition.

* Viewing personal street pixels.

Country and world progress may become additional Explorer Pro features in a later version.

---

# 30\. Settings

Public Android V1 settings include:

* Exploration haptics on or off.

* Competition enabled or disabled.

* Public nickname.

* Delete competition profile.

* Map-data management.

* Local recording management.

* Privacy information.

* Terms and competition rules.

When Explorer Pro is enabled in a build by feature flag and the user has entitlement, settings may also include GPX import and export and related track-management tools.

Public Android V1 does not require purchase, restore, or pricing settings.

The 25-metre exploration radius is not configurable in V1.

Internal HEALPix, GPS-filter, ownership-decay, and scoring parameters are not exposed as ordinary user settings.

---

# 31\. Error and empty states

## Location denied

Show the map and explain:

Location access is needed to record exploration automatically.

Provide actions to open system settings and continue browsing manually.

## Background location denied

Foreground recording remains available.

Explain that recording will pause when the application is no longer active.

## No downloaded map

Explain that the current area must be downloaded before reliable offline exploration works.

## Poor GPS accuracy

Show a subtle waiting state without creating interpolated exploration.

## Interrupted recording

Explain that part of the track may be missing.

Never fill the missing interval automatically.

## No selected exploration area

Personal pixels still work.

Area percentage and competition are unavailable until a supported settlement, administrative subdivision, or place boundary is present.

## No local competitors

Do not show an empty leaderboard.

Show personal qualification or leadership progress.

## No competition connectivity

Continue personal exploration and queue delayed aggregate updates.

## Route impossible under avoid-explored mode

Explain that no fully unexplored route is available and offer:

* Allow a small amount of explored routing.

* Return to normal routing.

---

# 32\. Product analytics

Analytics must be aggregate and privacy-conscious.

Raw GPS coordinates and tracks are never sent to product analytics.

## 32.1 Activation

Measure:

* Location permission granted.

* Background recording permission granted.

* First recording started.

* First pixel collected.

* First 10 pixels collected.

* First 100 metres explored.

* First recording completed.

## 32.2 Core engagement

Measure:

* Active recording sessions.

* New pixels collected per active week.

* Areas with measurable progress.

* First 25% milestone.

* First 50% milestone.

* First area completed.

* Prefer-unexplored routing usage.

* Avoid-explored routing usage.

## 32.3 Competition

Measure:

* Competition prompt viewed.

* Competition opt-in rate.

* Users qualifying for leadership.

* Users becoming boss.

* Areas becoming contested.

* Areas becoming unclaimed.

* Weekly city leaderboard usage.

## 32.4 Growth

Measure:

* Completion card generated.

* Share action initiated.

## 32.5 Monetization

Measure when Explorer Pro is enabled in a build:

* Explorer Pro information page viewed.

* GPX import usage.

* GPX export usage.

Purchase conversion and restoration metrics are post-V1.

---

# 33\. V1 success indicators

Public Android V1 is successful when it demonstrates that:

1. New users understand the red-to-green mechanic without a full tutorial.

2. Users deliberately start recording sessions.

3. Background recording works reliably with the screen off.

4. Users change their routes to collect new pixels.

5. Area percentages change frequently enough to feel rewarding.

6. Users return to continue an unfinished area.

7. Competition opt-in feels optional rather than coercive.

8. Sparse competition outside promoted cities does not feel broken.

9. Users can become and lose leadership in a comprehensible way.

10. GPS loss never creates kilometre-long false exploration lines.

11. Personal exploration remains dependable offline.

12. Street-pixel rendering remains acceptable during normal exploration use.

13. Shareable completion cards are used without exposing private location data.

Initial quantitative hypotheses:

* At least 70% of users who grant location access start a first recording.

* At least 70% of first recordings collect one pixel.

* At least 50% collect 100 metres during their first active session.

* At least 20% of activated users return within seven days.

* At least 10% of users shown the competition prompt opt in.

These are validation targets, not promises.

---

# 34\. Public V1 launch requirements

Public Android V1 is ready for release when all of the following are true.

## Core map and exploration

* Eligible OSM route filtering is implemented.

* Street pixels are generated deterministically.

* Red and green states persist locally.

* The 25-metre collection radius behaves consistently.

* Imported and live-explored pixels are distinguishable.

* Area percentages are correct for the installed map version.

* Versioned country-specific administrative-area configuration is applied deterministically.

* Core personal exploration works wherever compatible CoMaps data is available.

* The application does not restrict availability by city allowlist or pilot-only runtime behavior.

## Recording

* Sessions can be started, paused, resumed, and finished.

* Recording continues in the background where permission permits.

* Recording continues with the screen off.

* Session state is clearly visible.

* Interrupted sessions do not create false connecting lines.

* Recorded tracks can be inspected and deleted locally.

## GPS integrity

* Poor-accuracy samples are rejected.

* Implausible speed is rejected.

* Long jumps are rejected.

* Normal valid samples are interpolated safely.

* Signal loss does not paint a straight explored line.

* Pause and resume do not create connecting segments.

## Progress experience

* First-use guidance works.

* Area focus behaves predictably.

* 25%, 50%, and 100% milestones work.

* Completed areas have a clear visual state.

* City-level aggregate progress works.

* No achievement-history screen is required.

## Routing

* Standard routing works.

* Prefer-unexplored routing works.

* Avoid-explored routing handles impossible routes clearly and does not silently ignore the selected rule.

## Privacy and competition

* Competition is opt-in.

* Competition consent is separate from location permission.

* No raw GPS data is uploaded.

* Aggregate uploads are delayed and batched.

* Exact-location sharing is absent.

* Nearby-user discovery is absent.

* Pseudonymous identity creation works.

* Nickname restrictions and reporting work.

* Users can rename themselves within the stated limits.

* Users can delete competition data.

* Ownership calculations work.

* Server-side decay works.

* Areas can become unclaimed.

* Sparse ranking states preserve privacy.

* Weekly city rankings exclude imports.

## Offline and map updates

* Manual map updates work.

* Users are warned that percentages may change.

* Statistics are recalculated after updates.

* Competition uploads include map-data versions.

* Offline competition updates queue and later synchronize.

## Sharing

* Completion cards work without exposing routes, exact location, home location, or individual timestamps.

* Completion cards work without a competition profile.

* Share-card generation is available at 100% area completion.

## Explorer Pro and monetization

* Explorer Pro feature gates and entitlement abstraction are suitable for later paid activation.

* Public builds do not present a non-functional purchase action.

* Imported tracks cannot affect competition regardless of feature-flag state.

* Public Android V1 does not require Google Play Billing integration, purchase flow, or purchase restoration.

## Release governance

* Privacy policy accurately describes local and uploaded data.

* Competition consent text matches actual behavior.

* Terms cover public nicknames and rankings.

* Competition-profile deletion is operational.

* Nickname moderation and administrative reset are operational.

* Android store permissions and background-location disclosures are accurate.

* Analytics contain no raw location data.

## Quality

* Street-pixel rendering performance is acceptable during normal exploration use.

* Battery consumption is acceptable during active recording.

* Foreground haptics can be disabled.

* No critical exploration-data loss exists.

* No known path can reveal another user’s live or exact location.

---

# 35\. Post-V1 candidates

The following are deliberately deferred:

* iOS public release.

* StoreKit integration, iOS-specific permission flows, UI parity requirements, and iOS release gates.

* Explorer Pro purchasing, Google Play Billing integration, purchase restoration, pricing, and store entitlement validation.

* Country exploration percentages.

* World exploration percentages.

* Country and global public leaderboards.

* Cross-device accounts and profile recovery.

* Friends and private groups.

* Family leaderboards.

* Exact-location sharing.

* Nearby-user discovery.

* Live cooperative exploration.

* Exploration history over time.

* Achievement-history screens.

* Route generation for completing an area.

* Time-limited neighborhood events.

* Stronger anti-cheat and server verification.

* Signed or attested client statistics.

* Automatic map updates.

* Server-side map-version normalization.

* Travel-history visualization.

* Custom map themes.

* Additional haptic options.

* Exploration streaks.

* Additional milestone types.

* Curated discovery layers.

* Generated loop routes.

* Advanced heatmaps.

* Supporter subscriptions or donations.

Any future location-sharing or nearby-discovery feature requires a separate opt-in flow and privacy review.

---

# 36\. Final V1 product definition

Street Pixels public Android V1 is:

An offline-first map that turns nearby streets from red to green during explicit exploration sessions, tracks meaningful progress through real administrative neighborhoods, offers routes through unfamiliar streets, and optionally turns local exploration into a privacy-conscious competition.

The default experience is personal, quiet, and private.

The game layer is optional, contextual, delayed rather than live, and based on validated real exploration rather than artificial missions.

The product succeeds when users begin choosing streets because they are still red.