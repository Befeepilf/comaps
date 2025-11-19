# Release Management

## Prepare release

### A few days before the release
- [ ] start map generation
- [ ] raise release notes PR and ask for translations

### Shortly before the release day
- [ ] merge a "[planet] Update map data to xxxxxx" PR with new countries.txt
- [ ] merge a meta-php PR with new maps data version
- [ ] make sure new maps files are on all CDN nodes
- [ ] smoke test map data for World and some regions
- [ ] merge (translated) release notes PR

## Build a release for Android
## Shortly before the release day
- [ ] update GP metadata (descriptions, screenshots..) some days before the release

### On release day
- [ ] create a release branch release/2025.xx.yy
    - add a "[fdroid] Release version 2025.xx.yy-zz" commit
    - with a change to android/app/src/fdroid/play/version.yaml
    - take the version number from `tools/unix/version.sh` +1
    - e.g. with an added version bump commit it should match script's output
- [ ] tag the new release and push the tag to codeberg
    - the tag format is "v2025.08.13-4" (not the old "2025.08.13-4-android"!)
    - F-Droid will pickup the new tag (happens once a day)
- [ ] regenerate symbols
- [ ] regenerate drules
- [ ] update the world*.mwm

- [ ] build Google release bundle (`./gradlew bundleGoogleRelease`) - check the keys are Google upload / test (yes, they're the same at test keys)!
upload to GP and submit for a review

- [ ] build Codeberg release (`./gradlew -Parm32 -Parm64 assembleWebRelease`, e.g. no x86) - check the keys are prod Codeberg / main!
- [ ] create a Codeberg release from the tag, upload the build, add relnotes
- [ ] Upload the release to Google and submit for review
- [ ] announce the release in TG channel, upload the build


## Build a release for iOS

### Shortly before the release day
- [ ] **Add short iOS release notes to releases document**
    - They should consist of the most important things in general, that will be new in this version
    - Those will be used for the App Store
    - They will be translated in a pull request together with the short Android release notes
- [ ] **Add long iOS release notes to releases document**
    - They should consist only of iOS-specific things, that will be new in this version
    - Include credits to the persons, who added those new things
- [ ] **Add release with version number in App Store Connect**
    - The version number should be the planned release date in `yyyy.MM.dd` format (like _2025.08.30_)
- [ ] **Update metadata in App Store Connect**
    - Only include languages for which the App Store metadata has been completely translated

### On release day
- [ ] **Wait for the Android release to be prepared**
- [ ] **Pull the latest version form the repository**
    - The same last commit like for the Android release should be used
- [ ] **Edit versions and build number of the `CoMaps` and `CoMapsWidgetExtension` targets in Xcode**
    - Use the same versions and build number of the Android release
- [ ] **Select the build target _Any iOS Device_ in Xcode**
- [ ] **Clean the build folder in Xcode**
    - This can be done via _Product_ / _Clean Build Folder..._ in the menubar
- [ ] **Manually regenerate the styles**
    - `./tools/unix/generate_styles.sh`
- [ ] **Create the application archive in Xcode**
    - This can be done via _Product_ / _Archive_ in the menubar
- [ ] **Upload application to the App Store via the Xcode organizer**
    - Choose the freshly created application archive, press `Distribute App`, select `App Store Connect` and follow the steps until the app is successfully uploaded
- [ ] **Adjust the version in App Store Connect, if necessary**
    - It should be the same version used in Xcode and for the Android release
- [ ] **Add the short release notes in App Store Connect**
    - The translations can be found in a translation pull request specific for that version
    - For the languages, which are fully translated for the App Store, but are missing translated release notes, use the English release notes
- [ ] **Add the build to the release in App Store Connect**
    - It might take a few minutes for the uploaded build to actually show up
- [ ] **Submit the release to review**
    - Don't forget the actual submission step after clicking on _Add for Review_
- [ ] **Submit the same build to TestFlight review**
- [ ] **Wait for Apple approving the release and TestFlight build**

### After Apple's release approval
- [ ] **Also release the build on TestFlight**
- [ ] **Notify everybody in the Zulip topic for that release about the App Store availability**

## Shortly after the release

### Distribute to the French National library
 - [ ] **Build a google apk (with google flavord /web keys and only arm32/64 arches) for French National library**
 - [ ] **Upload it to their sftp**

### Update `json` for [TagInfo](https://taginfo.openstreetmap.org/)
- [ ] **Run script to generate file** 
  -  `python tools/python/generate_taginfo.py`
- [ ] **Raise PR to add updated file** 
    -  `data/taginfo.json`

### Announcements
- [ ] **Create a banner**
- [ ] **Upload banner to website news**
- [ ] **Announce the release to Telegram**
- [ ] **Create a post in social media**
    - Reddit
    - Mastodon
    - etc.
	
# Tools to upload metadata and screenshots on stores

## Apple App Store

### Upload metadata and screenshots to the App Store

Use [Forgejo Actions](../.forgejo/workflows/ios-release.yaml).

### Check metadata

Use [Forgejo Actions](../.forgejo/workflows/ios-check.yaml).

Local check:

```bash
./tools/python/check_store_metadata.py ios
```

### Downloading screenshots from the App Store

Get xcode/keys/appstore.json - App Store API Key.

Get screenshots/ - a repository with screenshots.

Download metadata:

```bash
cd xcode
./fastlane download_metadata
```

Download screenshots:

```bash
cd xcode
./fastlane download_screenshots
```

## Google Play

### Upload metadata and screenshots to Google Play

Use [Forgejo Actions](../.forgejo/workflows/android-release-metadata.yaml).

### Checking metadata

Use [Forgejo Actions](../.forgejo/workflows/android-check.yaml).

Checking locally:

```bash
./tools/python/check_store_metadata.py android
```

### Downloading metadata and screenshots from Google Play

Get `android/google-play.json` - Google Play API Key.

Get `screenshots/` - a repository with screenshots.

Download metadata:

```bash
./tools/android/download_googleplay_metadata.sh
```
