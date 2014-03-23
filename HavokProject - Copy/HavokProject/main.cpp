// include the basic windows header files and the Direct3D header file
#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "HavokUtilities.h"

#include <vector>

// define the screen resolution
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// include the Direct3D Library files
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

// global declarations
LPDIRECT3D9 d3d;
LPDIRECT3DDEVICE9 d3ddev;
LPDIRECT3DVERTEXBUFFER9 v_buffer = NULL;
LPDIRECT3DINDEXBUFFER9 i_buffer = NULL;

// function prototypes
void initD3D(HWND hWnd);
void render_frame(std::vector<hkpRigidBody*> bodies);
void cleanD3D(void);
void init_graphics(void);
void init_light(void);

struct CUSTOMVERTEX {FLOAT X, Y, Z; D3DVECTOR NORMAL;};
#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_NORMAL)

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


void addFixedSurface(hkpWorld* world, const hkVector4& position, const hkVector4& dimensions,
					 hkVector4& axis = hkVector4(0.0,0.0,1.0), const hkReal angle = hkReal(-0.1))
{
	//create box shape using given dimensions
	hkpConvexShape* shape = new hkpBoxShape(dimensions,0);

	//create rigid body information structure
	hkpRigidBodyCinfo rigidBodyInfo;

	//Motion_fixed means static element in game scene
	rigidBodyInfo.m_motionType = hkpMotion::MOTION_FIXED;
	rigidBodyInfo.m_shape = shape;
	rigidBodyInfo.m_position = position;
	rigidBodyInfo.m_mass = 0.0;
	axis.normalize3();
	hkVector4 m_faxis = axis;
	hkReal m_fangle = angle;
	rigidBodyInfo.m_rotation = hkQuaternion(m_faxis,m_fangle);

	//create new rigid body with supplied info
	hkpRigidBody* rigidBody = new hkpRigidBody(rigidBodyInfo);

	//add rigid body to physics world
	world->lock();
	world->addEntity(rigidBody);

	//decrease reference counter for rigid body and shape
	rigidBody->removeReference();
	shape->removeReference();

	world->unlock();
}


hkpRigidBody* addMovingBox(hkpWorld* world, const hkVector4& position, 
					 const hkVector4& dimensions)
{
	//addMovingBoxes function
	//creates moving boxes with specified position and dimensions

	//create box shape using given dimensions
	hkReal m_fhkConvexShapeRadius= hkReal(0.05);
	hkpShape* movingBodyShape = new hkpBoxShape(dimensions,m_fhkConvexShapeRadius);

	// Compute the inertia tensor from the shape
	hkpMassProperties m_massProperties;
	hkReal m_massOfBox = 1.0;
	hkpInertiaTensorComputer::computeShapeVolumeMassProperties(movingBodyShape, m_massOfBox, m_massProperties);

	//create rigid body information structure 
	hkpRigidBodyCinfo m_rigidBodyInfo;
	
	// Assign the rigid body properties
	m_rigidBodyInfo.m_position = position;
	m_rigidBodyInfo.m_mass = m_massProperties.m_mass;
	m_rigidBodyInfo.m_centerOfMass = m_massProperties.m_centerOfMass;
	m_rigidBodyInfo.m_inertiaTensor = m_massProperties.m_inertiaTensor;
	m_rigidBodyInfo.m_shape = movingBodyShape;
	m_rigidBodyInfo.m_motionType = hkpMotion::MOTION_BOX_INERTIA;

	//create new rigid body with supplied info
	hkpRigidBody* m_pRigidBody = new hkpRigidBody(m_rigidBodyInfo);

	//add rigid body to physics world
	world->lock();
	world->addEntity(m_pRigidBody);

	//decerase reference counter for rigid body and shape
	m_pRigidBody->removeReference();
	movingBodyShape->removeReference();

	world->unlock();

	return m_pRigidBody;
}



hkpRigidBody* addMovingBall(hkpWorld* world, const hkReal radius, const hkReal sphereMass,
				   const hkVector4& relPos,const hkVector4& velocity)
{
	hkpRigidBodyCinfo info;
	hkpMassProperties massProperties;
	hkpInertiaTensorComputer::computeSphereVolumeMassProperties(radius, sphereMass, massProperties);

	info.m_mass = massProperties.m_mass;
	info.m_centerOfMass  = massProperties.m_centerOfMass;
	info.m_inertiaTensor = massProperties.m_inertiaTensor;
	info.m_shape = new hkpSphereShape( radius );
	info.m_position = relPos;
	info.m_motionType  = hkpMotion::MOTION_BOX_INERTIA;

	info.m_qualityType = HK_COLLIDABLE_QUALITY_BULLET;


	hkpRigidBody* sphereRigidBody = new hkpRigidBody( info );
	//bowlingBall = sphereRigidBody;

	world->lock();
	world->addEntity( sphereRigidBody );

	sphereRigidBody->removeReference();
	info.m_shape->removeReference();

	sphereRigidBody->setLinearVelocity( velocity );

	world->unlock();

	return sphereRigidBody;
}




// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"WindowClass";

    RegisterClassEx(&wc);

    hWnd = CreateWindowEx(NULL, L"WindowClass", L"Our Direct3D Program",
                          WS_OVERLAPPEDWINDOW, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                          NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D
    initD3D(hWnd);

	// set up and initialize Havok

	HavokUtilities* havokUtilities = new HavokUtilities(true);

	//Array of rigid bodies
	std::vector<hkpRigidBody*> bodies;

	//Floor surface variables
	hkVector4 surfacePos = hkVector4(0,0,250.f);
	hkVector4 surfaceDim = hkVector4(50.0f,1.0f,250.0f);
	hkVector4 surfaceAxis = hkVector4(1.0,2.0,0.0);
	hkReal surfaceAngle = hkReal(0.0);

	//Add a fixed surface
	addFixedSurface(havokUtilities->getWorld(), surfacePos, surfaceDim, surfaceAxis,surfaceAngle);

	//Ball variables
	hkReal ballRadius = hkReal(7.f);
	hkReal ballMass = hkReal(20.f);
	hkVector4 ballPos = hkVector4(5.0f,10.0f,500.0f);
	hkVector4 ballVel = hkVector4(0.0f,4.9f,-60.0f);

	//Add a moving ball
	bodies.push_back(addMovingBall(havokUtilities->getWorld(), ballRadius, ballMass, ballPos, ballVel));

	//Pin box variables
	hkVector4 pinPos;
	hkVector4 pinDim = hkVector4(3.f,15.0f,3.f);

	//Add the pins
	for(int row=4;row>0;row--)
	{
		for(int pin=1;pin<=row;pin++)
		{
			pinPos = hkVector4(-25.f+pin*50.f/float(row+1), 16.f, 50.f-10.f*float(row));
			bodies.push_back(addMovingBox(havokUtilities->getWorld(), pinPos, pinDim));
		}
	}

	//create watch object - we will simulate for 30 seconds of real time
	hkStopwatch stopWatch;
	stopWatch.start();
	hkReal lastTime = stopWatch.getElapsedSeconds();

	//initialize fixed time step - one step every 1/60 of a second
	hkReal timestep = 1.f/60.f;

    // enter the main loop:

    MSG msg;

    while(TRUE)
    {
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(msg.message == WM_QUIT)
            break;
		//Do physics here
			//step the simulation and VDB
		havokUtilities->stepSimulation(timestep);
		havokUtilities->stepVisualDebugger(timestep);

		//Do physics logic here

		//pause until the actual time has passed
		while(stopWatch.getElapsedSeconds() < lastTime + timestep);
		{
			lastTime += timestep;
		}
		//Do Rendering here
        render_frame(bodies);
    }

    cleanD3D();

    return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            } break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}


// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);

    D3DPRESENT_PARAMETERS d3dpp;

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = SCREEN_WIDTH;
    d3dpp.BackBufferHeight = SCREEN_HEIGHT;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // create a device class using this information and the info from the d3dpp stuct
    d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &d3ddev);

    init_graphics();    // call the function to initialize the cube
    init_light();    // call the function to initialize the light and material

    d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE);    // turn on the 3D lighting
    d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on the z-buffer
    d3ddev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(50, 50, 50));    // ambient light
    d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);    // handle normals in scaling
}


// this is the function used to render a single frame
void render_frame(std::vector<hkpRigidBody*> bodies)
{
    d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    d3ddev->BeginScene();

    // select which vertex format we are using
    d3ddev->SetFVF(CUSTOMFVF);

    // set the view transform
    D3DXMATRIX matView;    // the view transform matrix
    D3DXMatrixLookAtLH(&matView,
    &D3DXVECTOR3 (0.0f, 500.0f, 10.0f),    // the camera position
    &D3DXVECTOR3 (0.0f, 0.0f, 0.0f),    // the look-at position
    &D3DXVECTOR3 (0.0f, 1.0f, 0.0f));    // the up direction
    d3ddev->SetTransform(D3DTS_VIEW, &matView);    // set the view transform to matView 

    // set the projection transform
    D3DXMATRIX matProjection;    // the projection transform matrix
    D3DXMatrixPerspectiveFovLH(&matProjection,
                               D3DXToRadian(45),    // the horizontal field of view
                               (FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT, // aspect ratio
                               1.0f,    // the near view-plane
                               1000.0f);    // the far view-plane
    d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection);    // set the projection

	
	for(int i=0;i<bodies.size();i++)
	{

		hkQuaternion rot = bodies[i]->getRotation(); 
		D3DXQUATERNION quatRot((float)rot(0),(float)rot(1),(float)rot(2),(float)rot(3));

		// set the world transform
		D3DXMATRIX matRotTrans; //The final matrix
		D3DXMATRIX matRotate;	//The world rotation matrix
		D3DXMatrixRotationQuaternion(&matRotate,&quatRot);

		hkVector4 pos = bodies[i]->getPosition();
		D3DXVECTOR3 posVect((float)pos(0),(float)pos(1),(float)pos(2));

		D3DXMATRIX matTranslate;    // the world transform matrix
		D3DXMatrixTranslation(&matTranslate,posVect.x,posVect.y,posVect.z); //Set ball position to follow Havok physics
		D3DXMatrixMultiply(&matRotTrans,&matRotate,&matTranslate);
		d3ddev->SetTransform(D3DTS_WORLD, &(matRotTrans));    // set the world transform

		// select the vertex and index buffers to use
		d3ddev->SetStreamSource(0, v_buffer, 0, sizeof(CUSTOMVERTEX));
		d3ddev->SetIndices(i_buffer);

		// draw the cube
		d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);

	}

    d3ddev->EndScene(); 

    d3ddev->Present(NULL, NULL, NULL, NULL);
}


// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
    v_buffer->Release();    // close and release the vertex buffer
    i_buffer->Release();    // close and release the vertex buffer
    d3ddev->Release();    // close and release the 3D device
    d3d->Release();    // close and release Direct3D
}


// this is the function that puts the 3D models into video RAM
void init_graphics(void)
{
    // create the vertices using the CUSTOMVERTEX struct
    CUSTOMVERTEX vertices[] =
    {
        { -3.0f, -3.0f, 3.0f,  0.0f, 0.0f, 1.0f, },    // side 1
        { 3.0f, -3.0f, 3.0f,  0.0f, 0.0f, 1.0f, },
        { -3.0f, 3.0f, 3.0f,  0.0f, 0.0f, 1.0f, },
        { 3.0f, 3.0f, 3.0f,  0.0f, 0.0f, 1.0f, },

        { -3.0f, -3.0f, -3.0f,  0.0f, 0.0f, -1.0f, },    // side 2
        { -3.0f, 3.0f, -3.0f,  0.0f, 0.0f, -1.0f, },
        { 3.0f, -3.0f, -3.0f,  0.0f, 0.0f, -1.0f, },
        { 3.0f, 3.0f, -3.0f,  0.0f, 0.0f, -1.0f, },

        { -3.0f, 3.0f, -3.0f,  0.0f, 1.0f, 0.0f, },    // side 3
        { -3.0f, 3.0f, 3.0f,  0.0f, 1.0f, 0.0f, },
        { 3.0f, 3.0f, -3.0f,  0.0f, 1.0f, 0.0f, },
        { 3.0f, 3.0f, 3.0f,  0.0f, 1.0f, 0.0f, },

        { -3.0f, -3.0f, -3.0f,  0.0f, -1.0f, 0.0f, },    // side 4
        { 3.0f, -3.0f, -3.0f,  0.0f, -1.0f, 0.0f, },
        { -3.0f, -3.0f, 3.0f,  0.0f, -1.0f, 0.0f, },
        { 3.0f, -3.0f, 3.0f,  0.0f, -1.0f, 0.0f, },

        { 3.0f, -3.0f, -3.0f,  1.0f, 0.0f, 0.0f, },    // side 5
        { 3.0f, 3.0f, -3.0f,  1.0f, 0.0f, 0.0f, },
        { 3.0f, -3.0f, 3.0f,  1.0f, 0.0f, 0.0f, },
        { 3.0f, 3.0f, 3.0f,  1.0f, 0.0f, 0.0f, },

        { -3.0f, -3.0f, -3.0f,  -1.0f, 0.0f, 0.0f, },    // side 6
        { -3.0f, -3.0f, 3.0f,  -1.0f, 0.0f, 0.0f, },
        { -3.0f, 3.0f, -3.0f,  -1.0f, 0.0f, 0.0f, },
        { -3.0f, 3.0f, 3.0f,  -1.0f, 0.0f, 0.0f, },
    };

    // create a vertex buffer interface called v_buffer
    d3ddev->CreateVertexBuffer(24*sizeof(CUSTOMVERTEX),
                               0,
                               CUSTOMFVF,
                               D3DPOOL_MANAGED,
                               &v_buffer,
                               NULL);

    VOID* pVoid;    // a void pointer

    // lock v_buffer and load the vertices into it
    v_buffer->Lock(0, 0, (void**)&pVoid, 0);
    memcpy(pVoid, vertices, sizeof(vertices));
    v_buffer->Unlock();

    // create the indices using an int array
    short indices[] =
    {
        0, 1, 2,    // side 1
        2, 1, 3,
        4, 5, 6,    // side 2
        6, 5, 7,
        8, 9, 10,    // side 3
        10, 9, 11,
        12, 13, 14,    // side 4
        14, 13, 15,
        16, 17, 18,    // side 5
        18, 17, 19,
        20, 21, 22,    // side 6
        22, 21, 23,
    };

    // create an index buffer interface called i_buffer
    d3ddev->CreateIndexBuffer(36*sizeof(short),
                              0,
                              D3DFMT_INDEX16,
                              D3DPOOL_MANAGED,
                              &i_buffer,
                              NULL);

    // lock i_buffer and load the indices into it
    i_buffer->Lock(0, 0, (void**)&pVoid, 0);
    memcpy(pVoid, indices, sizeof(indices));
    i_buffer->Unlock();
}


// this is the function that sets up the lights and materials
void init_light(void)
{
    D3DLIGHT9 light;
    D3DMATERIAL9 material;

    ZeroMemory(&light, sizeof(light));
    light.Type = D3DLIGHT_POINT;    // make the light type 'point light'
    light.Diffuse = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);
    light.Position = D3DXVECTOR3(0.0f, 5.0f, 0.0f);
    light.Range = 100.0f;    // a range of 100
    light.Attenuation0 = 0.0f;    // no constant inverse attenuation
    light.Attenuation1 = 0.125f;    // only .125 inverse attenuation
    light.Attenuation2 = 0.0f;    // no square inverse attenuation

    d3ddev->SetLight(0, &light);
    d3ddev->LightEnable(0, TRUE);

    ZeroMemory(&material, sizeof(D3DMATERIAL9));
    material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    material.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

    d3ddev->SetMaterial(&material);
}