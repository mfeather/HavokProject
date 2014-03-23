
// include the basic windows header files and the Direct3D header file
#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>


class DxModel
{
private:
	void LoadMesh(LPCWSTR meshName)
	{
    LPD3DXBUFFER bufMaterial;

    D3DXLoadMeshFromX(meshName,				// load this file
                      D3DXMESH_SYSTEMMEM,   // load the mesh into system memory
                      d3ddev,				// the Direct3D Device
                      NULL,					// we aren't using adjacency
                      &bufMaterial,			// put the materials here
                      NULL,					// we aren't using effect instances
                      &numMaterials,		// the number of materials in this model
                      &mesh);				// put the mesh here

    // retrieve the pointer to the buffer containing the material information
    D3DXMATERIAL* tempMaterials = (D3DXMATERIAL*)bufMaterial->GetBufferPointer();

    // create a new material buffer and texture for each material in the mesh
    material = new D3DMATERIAL9[numMaterials];
    texture = new LPDIRECT3DTEXTURE9[numMaterials];

    for(DWORD i = 0; i < numMaterials; i++)    // for each material...
    {
        material[i] = tempMaterials[i].MatD3D;    // get the material info
        material[i].Ambient = material[i].Diffuse;    // make ambient the same as diffuse
        // if there is a texture to load, load it
        if(FAILED(D3DXCreateTextureFromFileA(d3ddev,
                                             tempMaterials[i].pTextureFilename,
                                             &texture[i])))
        texture[i] = NULL;    // if there is no texture, set the texture to NULL
    }

	};

	LPDIRECT3DDEVICE9 d3ddev;		// the pointer to the device class
	LPD3DXMESH mesh;				// the ball pointer
	D3DMATERIAL9* material;			// define the material object
	LPDIRECT3DTEXTURE9* texture;    // a pointer to a texture
	DWORD numMaterials;				// stores the number of materials in the mesh
	float modelScale;				// the uniform scale of the model
	D3DXMATRIX matScale;			// a matrix to store the scaling of the model
	D3DXMATRIX matTranslate;		// a matrix to store the translation of the model
    D3DXMATRIX matRotation;			// a matrix to store the rotation for each triangle
public:
	DxModel(LPDIRECT3DDEVICE9 device,LPCWSTR meshName,
		D3DXVECTOR3 initPos = D3DXVECTOR3(0.0f,0.0f,0.0f),float initScale=1.0f)
	{
		d3ddev = device;
		LoadMesh(meshName);
		position = initPos;
		modelScale = initScale;
	};
	void Update()
	{
		D3DXMatrixScaling(&matScale,modelScale,modelScale,modelScale);  //Scale the model

		D3DXMatrixTranslation(&matTranslate,position.x,position.y,position.z);  // the translation matrix

		D3DXMatrixRotationQuaternion(&matRotation, &modelRotation);    // the rotation matrix

	};
	void Draw(LPDIRECT3DDEVICE9 d3ddev)
	{
		D3DXMATRIX matWorld;							   // a matrix to store the total of all transforms
		matWorld = matScale * matRotation * matTranslate;   // multiply all the transforms and store in world matrix
		d3ddev->SetTransform(D3DTS_WORLD, &(matWorld));    // set the world transform

		  // draw the model
		for(DWORD i = 0; i < numMaterials; i++)    // loop through each subset
		{
			d3ddev->SetMaterial(&material[i]);    // set the material for the subset
			if(texture[i] != NULL)    // if the subset has a texture (if texture is not NULL)
				d3ddev->SetTexture(0, texture[i]);    // ...then set the texture

			mesh->DrawSubset(i);    // draw the subset
		}
	};
	void Clean()
	{
		mesh->Release();	// close and release the mesh
	};
	D3DXVECTOR3 position;			// position vector of the model
	D3DXQUATERNION modelRotation;   // the quaternion rotation of the model
};