package app.organicmaps.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.HashMap;
import java.util.Map;
import javax.xml.parsers.DocumentBuilderFactory;
import org.junit.Test;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

public class PublicManifestAssertionsTest
{
  private static final String ANDROID_NS = "http://schemas.android.com/apk/res/android";

  @Test
  public void accessBackgroundLocation_absentFromPublicManifests() throws Exception
  {
    for (File manifest : publicManifests())
      assertFalse(manifest.getPath(), readUtf8(manifest).contains("ACCESS_BACKGROUND_LOCATION"));
  }

  @Test
  public void addFriendIntentFilters_absentFromPublicManifests() throws Exception
  {
    Document appManifest = parse(appManifest());
    NodeList dataNodes = appManifest.getElementsByTagName("data");
    for (int i = 0; i < dataNodes.getLength(); i++)
    {
      Element data = (Element) dataNodes.item(i);
      assertFalse(data.getAttributeNS(ANDROID_NS, "host").equals("add-friend"));
      assertFalse(data.getAttributeNS(ANDROID_NS, "pathPrefix").equals("/add-friend"));
      assertFalse(data.getAttributeNS(ANDROID_NS, "path").startsWith("/add-friend"));
    }
    assertFalse(readUtf8(appManifest()).contains("add-friend"));
  }

  @Test
  public void locationForegroundServiceTypes_matchTrackRecordingAndNavigation() throws Exception
  {
    Map<String, String> types = serviceForegroundTypes(parse(appManifest()));
    assertEquals("location", types.get("app.organicmaps.location.TrackRecordingService"));
    assertEquals("location", types.get("app.organicmaps.routing.NavigationService"));
    assertEquals("dataSync", types.get("app.organicmaps.downloader.DownloaderService"));
  }

  @Test
  public void foregroundServiceLocationPermission_declared() throws Exception
  {
    assertTrue(usesPermission(parse(appManifest()), "android.permission.FOREGROUND_SERVICE_LOCATION"));
    assertTrue(usesPermission(parse(appManifest()), "android.permission.FOREGROUND_SERVICE_DATA_SYNC"));
  }

  @Test
  public void friendsCapability_offInPublicV1()
  {
    assertFalse(FriendSettingsVisibility.friendsCapabilityEnabled());
    assertFalse(FriendSettingsVisibility.showAddFriendOnboarding(
        FriendSettingsVisibility.friendsCapabilityEnabled()));
  }

  private static File[] publicManifests()
  {
    return new File[]{appManifest(), sdkManifest()};
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
    return Files.readString(file.toPath(), StandardCharsets.UTF_8);
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
