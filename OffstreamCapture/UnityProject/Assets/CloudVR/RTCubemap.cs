using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;
using UnityEngine.Rendering;

public class RTCubemap : MonoBehaviour
{

    private static int sqr = 1280;
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
    
    private RenderTexture leftRender;
    private RenderTexture rightRender;
    private RenderTexture topRender;
    private RenderTexture bottomRender;
    private RenderTexture frontRender;
    private RenderTexture backRender;

	Transform cachedTransform;


    private void Start()
    {
        leftRender = new RenderTexture(sqr, sqr, 24, RenderTextureFormat.ARGB32);
        rightRender = new RenderTexture(sqr, sqr, 24, RenderTextureFormat.ARGB32);
        topRender = new RenderTexture(sqr, sqr, 24, RenderTextureFormat.ARGB32);
        bottomRender = new RenderTexture(sqr, sqr, 24, RenderTextureFormat.ARGB32);
        frontRender = new RenderTexture(sqr, sqr, 24, RenderTextureFormat.ARGB32);
        backRender = new RenderTexture(sqr, sqr, 24, RenderTextureFormat.ARGB32);
        
        leftCam.targetTexture = leftRender;
        rightCam.targetTexture = rightRender;
        topCam.targetTexture = topRender;
        bottomCam.targetTexture = bottomRender;
        frontCam.targetTexture = frontRender;
        backCam.targetTexture = backRender;

		cachedTransform = transform;

		//SetupDebugDelegate();
		StartPlugin();

		//StartCoroutine (MoveCamera());
    }


	IEnumerator MoveCamera(){//to simulate a moving camera
		int i = 0;
		while (true) {

			cachedTransform.position += Vector3.right;
			i++;
			if (i > 300) {
				
				break;
			}

			yield return null;
		}
	}


	IEnumerator CallPluginAtEndOfFrame(){
		yield return new WaitForEndOfFrame ();
		SendTextureIdsToPlugin(leftRender.GetNativeTexturePtr(), 
								rightRender.GetNativeTexturePtr(), 
								topRender.GetNativeTexturePtr(), 
								bottomRender.GetNativeTexturePtr(), 
								frontRender.GetNativeTexturePtr(),
								backRender.GetNativeTexturePtr());
		
		GL.IssuePluginEvent(InitGraphicsEventFunc(), 0);
		yield return new WaitForEndOfFrame ();
		
		while(true){
			yield return new WaitForEndOfFrame ();
			GL.IssuePluginEvent(GetRenderEventFunc(), 0);
		}
	}

	void StartPlugin(){
		
		const string windows = "Windows";
		string ErrorMessage = "";
		const string graphicsNotSuported = "Please select the correct option at Build Settings->Player Settings->Other Settings->Auto Graphics API";
		const string restartUnity = "And restart Unity!";

 		if (SystemInfo.operatingSystem.StartsWith (windows)) {
			if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Direct3D11) {
				ErrorMessage = "Only Direct3D11 is supported."+graphicsNotSuported+" for Windows."+restartUnity;
			}
		} else {
			ErrorMessage = "Only Windows is suported!";
		}


		if(ErrorMessage.Length == 0){
			Application.runInBackground = true;

			StartCoroutine (CallPluginAtEndOfFrame());
		} else {
			Debug.LogError(ErrorMessage);
			
		}
	}

	void OnApplicationQuit ()
	{
		StopApplication();
	}
	
	[DllImport ("GridUnityPlugin")]
	static extern void SendTextureIdsToPlugin(IntPtr texLeftId, IntPtr texRightId, IntPtr texTopId, IntPtr texBottomId, IntPtr texFrontId, IntPtr texBackId);

	[DllImport ("GridUnityPlugin")]
	static extern IntPtr InitGraphicsEventFunc ();

	[DllImport ("GridUnityPlugin")]
	static extern IntPtr GetRenderEventFunc ();

	[DllImport ("GridUnityPlugin")]
	static extern IntPtr StopApplication();


	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void DebugDelegate(string message);

	[DllImport ("GridUnityPlugin")]
	static extern void SetDebugFunction (IntPtr fp);

	static void CallbackFunction(string message){
		Debug.Log("--PluginCallback--: "+message);
	}

	//call this ,if you want to see debug messages ,from the C++ plugin ( however , when using it, the Unity will block on closing ,when .cs scripts calls take place on the rendering thread)
	void SetupDebugDelegate(){
		DebugDelegate callbackDelegate = new DebugDelegate(CallbackFunction);
		IntPtr intPtrDelegate = Marshal.GetFunctionPointerForDelegate(callbackDelegate);
		SetDebugFunction(intPtrDelegate);
	}

}
