package app.organicmaps.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import javax.xml.parsers.DocumentBuilderFactory;
import org.junit.Test;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

public class PublicManifestAssertionsTest
{
  private static final String ANDROID_NS = "http://schemas.android.com/apk/res/android";
  private static final String TOOLS_NS = "http://schemas.android.com/tools";
  private static final String ABL = "android.permission.ACCESS_BACKGROUND_LOCATION";

  @Test
  public void accessBackgroundLocation_absentFromMergedManifests() throws Exception
  {
    List<File> merged = mergedManifests();
    assertFalse("merged manifest missing; unit tests must run process*MainManifest",
                merged.isEmpty());
    for (File manifest : merged)
    {
      assertFalse(manifest.getPath(), usesPermission(parse(manifest), ABL));
      assertFalse(manifest.getPath(), readUtf8(manifest).contains("ACCESS_BACKGROUND_LOCATION"));
    }
  }

  @Test
  public void accessBackgroundLocation_sourceOnlyRemovesLibraryMerge() throws Exception
  {
    for (File manifest : sourceManifests())
    {
      Document document = parse(manifest);
      NodeList nodes = document.getElementsByTagName("uses-permission");
      for (int i = 0; i < nodes.getLength(); i++)
      {
        Element element = (Element) nodes.item(i);
        if (!ABL.equals(element.getAttributeNS(ANDROID_NS, "name")))
          continue;
        assertEquals(manifest.getPath(), "remove", element.getAttributeNS(TOOLS_NS, "node"));
      }
    }
  }

  @Test
  public void addFriendIntentFilters_absentFromSourceAndMergedManifests() throws Exception
  {
    List<File> all = new ArrayList<>();
    all.addAll(sourceManifests());
    List<File> merged = mergedManifests();
    assertFalse("merged manifest missing; unit tests must run process*MainManifest",
                merged.isEmpty());
    all.addAll(merged);
    for (File manifest : all)
    {
      Document document = parse(manifest);
      NodeList dataNodes = document.getElementsByTagName("data");
      for (int i = 0; i < dataNodes.getLength(); i++)
      {
        Element data = (Element) dataNodes.item(i);
        assertFalse(manifest.getPath(),
                    "add-friend".equalsIgnoreCase(data.getAttributeNS(ANDROID_NS, "host")));
        assertFalse(manifest.getPath(),
                    "/add-friend".equalsIgnoreCase(data.getAttributeNS(ANDROID_NS, "pathPrefix")));
        String path = data.getAttributeNS(ANDROID_NS, "path");
        assertFalse(manifest.getPath(),
                    path.toLowerCase(Locale.ROOT).startsWith("/add-friend"));
        String pattern = data.getAttributeNS(ANDROID_NS, "pathPattern");
        assertFalse(manifest.getPath(),
                    pattern.toLowerCase(Locale.ROOT).contains("add-friend"));
      }
      assertFalse(manifest.getPath(), readUtf8(manifest).contains("add-friend"));
    }
  }

  @Test
  public void locationForegroundServiceTypes_matchTrackRecordingAndNavigation() throws Exception
  {
    List<File> toCheck = new ArrayList<>();
    toCheck.add(appManifest());
    toCheck.addAll(mergedManifests());
    assertFalse("merged manifest missing; unit tests must run process*MainManifest",
                mergedManifests().isEmpty());
    for (File manifest : toCheck)
    {
      Map<String, String> types = serviceForegroundTypes(parse(manifest));
      assertEquals(manifest.getPath(), "location",
                   types.get("app.organicmaps.location.TrackRecordingService"));
      assertEquals(manifest.getPath(), "location",
                   types.get("app.organicmaps.routing.NavigationService"));
      assertEquals(manifest.getPath(), "dataSync",
                   types.get("app.organicmaps.downloader.DownloaderService"));
    }
  }

  @Test
  public void foregroundServiceLocationPermission_declaredInMergedManifest() throws Exception
  {
    List<File> merged = mergedManifests();
    assertFalse("merged manifest missing; unit tests must run process*MainManifest",
                merged.isEmpty());
    for (File manifest : merged)
    {
      Document document = parse(manifest);
      assertTrue(manifest.getPath(),
                 usesPermission(document, "android.permission.FOREGROUND_SERVICE_LOCATION"));
      assertTrue(manifest.getPath(),
                 usesPermission(document, "android.permission.FOREGROUND_SERVICE_DATA_SYNC"));
    }
  }

  @Test
  public void friendsCapability_offInPublicV1()
  {
    assertFalse(FriendSettingsVisibility.friendsCapabilityEnabled());
    assertFalse(FriendSettingsVisibility.showAddFriendOnboarding(
        FriendSettingsVisibility.friendsCapabilityEnabled()));
    assertFalse(ExploreDeepLink.shouldPresentAddFriendOnboarding(null));
  }

  private static List<File> sourceManifests()
  {
    File appSrc = appManifest().getParentFile().getParentFile();
    File[] flavorDirs = appSrc.listFiles(File::isDirectory);
    assertNotNull(flavorDirs);
    Arrays.sort(flavorDirs, Comparator.comparing(File::getName));
    List<File> out = new ArrayList<>();
    for (File flavorDir : flavorDirs)
    {
      File manifest = new File(flavorDir, "AndroidManifest.xml");
      if (manifest.isFile())
        out.add(manifest);
    }
    out.add(sdkManifest());
    assertTrue("expected app flavor manifests", out.size() >= 3);
    return out;
  }

  private static List<File> mergedManifests()
  {
    File appDir = appManifest().getParentFile().getParentFile().getParentFile();
    File intermediates = new File(appDir, "build/intermediates");
    List<File> out = new ArrayList<>();
    collectMergedManifests(new File(intermediates, "merged_manifest"), out);
    collectMergedManifests(new File(intermediates, "merged_manifests"), out);
    out.sort(Comparator.comparing(File::getPath));
    List<File> googleDebug = new ArrayList<>();
    for (File file : out)
    {
      if (file.getPath().contains("googleDebug"))
        googleDebug.add(file);
    }
    return googleDebug.isEmpty() ? out : googleDebug;
  }

  private static void collectMergedManifests(File dir, List<File> out)
  {
    if (!dir.isDirectory())
      return;
    File[] children = dir.listFiles();
    if (children == null)
      return;
    for (File child : children)
    {
      if (child.isDirectory())
        collectMergedManifests(child, out);
      else if ("AndroidManifest.xml".equals(child.getName()))
        out.add(child);
    }
  }

  private static File appManifest()
  {
    return findRepoFile("android/app/src/main/AndroidManifest.xml");
  }

  private static File sdkManifest()
  {
    return findRepoFile("android/sdk/src/main/AndroidManifest.xml");
  }

  private static File findRepoFile(String relative)
  {
    File dir = new File(System.getProperty("user.dir")).getAbsoluteFile();
    while (dir != null)
    {
      File candidate = new File(dir, relative);
      if (candidate.isFile())
        return candidate;
      dir = dir.getParentFile();
    }
    if (relative.contains("/app/"))
    {
      File moduleRelative = new File("src/main/AndroidManifest.xml");
      if (moduleRelative.isFile())
        return moduleRelative.getAbsoluteFile();
    }
    if (relative.contains("/sdk/"))
    {
      File sdkRelative = new File("../sdk/src/main/AndroidManifest.xml");
      if (sdkRelative.isFile())
        return sdkRelative.getAbsoluteFile();
    }
    throw new AssertionError("missing " + relative + " from " + System.getProperty("user.dir"));
  }

  private static String readUtf8(File file) throws IOException
  {
    try (FileInputStream in = new FileInputStream(file))
    {
      byte[] buffer = new byte[(int) file.length()];
      int offset = 0;
      while (offset < buffer.length)
      {
        int read = in.read(buffer, offset, buffer.length - offset);
        if (read < 0)
          break;
        offset += read;
      }
      return new String(buffer, 0, offset, StandardCharsets.UTF_8);
    }
  }

  private static Document parse(File file) throws Exception
  {
    DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
    factory.setNamespaceAware(true);
    factory.setFeature("http://apache.org/xml/features/disallow-doctype-decl", true);
    Document document = factory.newDocumentBuilder().parse(file);
    assertNotNull(document.getDocumentElement());
    return document;
  }

  private static boolean usesPermission(Document document, String permission)
  {
    NodeList nodes = document.getElementsByTagName("uses-permission");
    for (int i = 0; i < nodes.getLength(); i++)
    {
      Element element = (Element) nodes.item(i);
      if (permission.equals(element.getAttributeNS(ANDROID_NS, "name")))
        return true;
    }
    return false;
  }

  private static Map<String, String> serviceForegroundTypes(Document document)
  {
    Map<String, String> types = new HashMap<>();
    NodeList nodes = document.getElementsByTagName("service");
    for (int i = 0; i < nodes.getLength(); i++)
    {
      Element element = (Element) nodes.item(i);
      String name = element.getAttributeNS(ANDROID_NS, "name");
      if (name.startsWith("."))
        name = "app.organicmaps" + name;
      types.put(name, element.getAttributeNS(ANDROID_NS, "foregroundServiceType"));
    }
    return types;
  }
}
