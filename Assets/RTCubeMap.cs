using UnityEngine;
using System.Collections;

public class RTCubeMap : MonoBehaviour
{

    private static int sqr = 900;
    private Texture2D cubemap;

    [SerializeField]
    private Camera leftCam;
    [SerializeField]
    private Camera rightCam;
    [SerializeField]
    private Camera topCam;
    [SerializeField]
    private Camera bottomCam;
    [SerializeField]
    private Camera frontCam;
    [SerializeField]
    private Camera backCam;

    private void Start()
    {
        cubemap = new Texture2D(sqr * 3, sqr * 2, TextureFormat.RGB24, false);
    }

    private void Update()
    {
        TakeSS(backCam, 0, sqr);
        TakeSS(topCam, sqr, sqr);
        TakeSS(bottomCam, sqr * 2, sqr);
        TakeSS(rightCam, 0, 0);
        TakeSS(frontCam, sqr, 0);
        TakeSS(leftCam, sqr * 2, 0);
        cubemap.Apply();
        GameObject.Find("RawImage").GetComponent<UnityEngine.UI.RawImage>().texture = cubemap;
    }

    private void TakeSS(Camera cam, int destX, int destY)
    {
        RenderTexture rendTex = new RenderTexture(sqr, sqr, 24);
        cam.targetTexture = rendTex;
        cam.Render();

        RenderTexture.active = rendTex;
        // false, meaning no need for mipmaps
        cubemap.ReadPixels(new Rect(0, 0, sqr, sqr), destX, destY);

        RenderTexture.active = null; //can help avoid errors
        cam.targetTexture = null;
        // consider ... Destroy(tempRT);
    }

}
