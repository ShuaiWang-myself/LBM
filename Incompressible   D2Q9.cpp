//=========================================================
//---------------------------------------------------------
// ----- Header file of the D2Q9 model -----
//---------------------------------------------------------
//File name: D2Q9.h
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <math.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <time.h>

using namespace std;

#define Nx 400     // number of cells in the x-direction
#define Ny 200     // number of cells in the y-direction
#define Nx1 (Nx+1)
#define Ny1 (Ny+1)
#define L 10
//(Ny+1)   // width of the cavity
#define Q 9 // number of discrete velocities
#define rho0 1.0   // initial density
#define ux0 0.0   // initial velocity component in x direction
#define uuu 0.1
#define uy0 0.0   // initial velocity component in y direction
#define uw 0.1
#define Re 1000.0
int cx[Q] = { 0, 1, 0, -1, 0, 1, -1, -1, 1 };
int cy[Q] = { 0, 0, 1, 0, -1, 1, 1, -1, -1 };
int a[Nx][Ny][3];
int b[2 * Nx][2 * Ny][3];
double f[Ny1][Nx1][Q]; //array of the distribution functions (DFs)
double f_post[Ny1][Nx1][Q]; // array of the post-collision DFs
double rho[Ny1][Nx1], ux[Ny1][Nx1], uy[Ny1][Nx1];
// arrays of fluid density and velocity
double tau; // relaxation time for BGK model
double s[Q]; // relaxation rates for MRT model
double D[Q] = { 9, 36, 36, 6, 12, 6, 12, 4, 4 }; // D = M*MT 

double w[Q] = { 4.0 / 9,1.0 / 9,1.0 / 9,1.0 / 9,1.0 / 9,1.0 / 36,1.0 / 36,
1.0 / 36,1.0 / 36 }; // the weights in the EDF
int rc[Q] = { 0,3,4,1,2,7,8,5,6 }; // index of reversed velocity
void Init_Eq(void); //Initialization
double feq(double RHO, double U, double V, int k);
// Equilibrium distribution function
void Coll_BGK(void); // BGK collision
void Coll_MRT(void); // MRT collision
double meq(double RHO, double U, double V, int k);
// Equilibrium momenta
void Streaming(void); // Streaming
void Den_Vel(void); // Fluid variables
void Bounce_back(void); // Bounce-back boundary condition
void zou_he();
double Err(void); // Difference in velocity field
double u0[Ny1][Nx1], v0[Ny1][Nx1];
void outputdata(int n);// Output simulation data
					   //=========================================================
					   //=========================================================

class Grid
{
public:
	struct vec3 {
		float x;
		float y;
		float z;

		vec3() {
			x = 0;
			y = 0;
			z = 0;
		}

		vec3(float xx, float yy, float zz) {
			x = xx;
			y = yy;
			z = zz;
		}
	};
};





void main()
{
	int n, M2, N2;
	double err;
	M2 = Ny / 2; N2 = Nx / 2;
	n = 0;
	err = 1.0;
	tau = 3 * L*uw / Re + 0.5; // relaxation time for BGK
	cout << tau << endl;
	s[7] = s[8] = 1.0 / tau; s[0] = s[3] = s[5] = 0.0; s[4] = s[6] = 1.05; s[1] = 1.1; s[2] = 1.05; // relaxation rates for MRT
	Init_Eq();
	for (int z; z = 30000; z++)
	{
		n++;
		//Coll_BGK(); //BGK collision
		Coll_MRT();			// Coll_MRT(); //MRT collision
							//	zou_he();
		Streaming(); // Streaming
		Bounce_back(); // No-Slip boundary condition
		Den_Vel(); // Fluid variables
		if (n % 5 == 0)
		{
			err = Err(); // Velocity differences between two successive 1000 steps
			printf("err=%e ux_center=%e uy_center=%e k=%d\n", err, ux[M2][N2], uy[M2][N2], n); // Display some results
		}
		outputdata(n); // Output simulation data
	}

}
//=========================================================
//-------------------------------------------------------------------
// Subroutine: initialization with the equilibrium method
//------------------------------------------------------------------
//


//function: u->RGB
Grid::vec3 mapVelocityToColor(float uu)
{
	float u = uu;
	float normalizedVelocity = (u) / (uuu * 1);
	Grid::vec3 result;
	if (normalizedVelocity <= 0.25) {
		result = Grid::vec3(0, (normalizedVelocity / 0.25f), 1);
	}
	else if (normalizedVelocity <= 0.5) {
		result = Grid::vec3((normalizedVelocity - 0.25f) / 0.5f, 1, (0.5f - normalizedVelocity) / 0.25f);
	}
	else if (normalizedVelocity <= 0.625) {
		result = Grid::vec3((normalizedVelocity - 0.25f) / 0.5f, 1, 0);
	}
	else if (normalizedVelocity <= 0.75) {
		result = Grid::vec3((normalizedVelocity - 0.25f) / 0.5f, (0.9f - normalizedVelocity) / 0.275f, 0);
	}
	else if (normalizedVelocity <= 0.9) {
		result = Grid::vec3(1, (0.9f - normalizedVelocity) / 0.275f, 0);
	}
	//	else if (normalizedVelocity <= 1) {
	//	result = Grid::vec3(1, 0,  (1.f - normalizedVelocity) / 0.1f);
	//}
	else {
		result = Grid::vec3(1, 0, 0);
	}
	return result;
}

void Init_Eq()
{
	int j, i, k;
	for (j = 0; j <= Ny; j++)
		for (i = 0; i <= Nx; i++)
		{
			rho[j][i] = 1;
			ux[j][i] = ux0;
			uy[j][i] = uy0;
			if (i == 0)
				if (j <= 0.6*Ny&&j >= 0.4*Ny)
					ux[j][i] = uuu;

			for (k = 0; k<Q; k++)
				f[j][i][k] = feq(rho[j][i], ux[j][i], uy[j][i], k);
		}
}
//========================================================
//=========================================================
//-----------------------------------------------------------------
// Subroutine: calculation the equilibrium distribution
//----------------------------------------------------------------
//
double feq(double RHO, double U, double V, int k)
{
	double cu, U2;
	cu = cx[k] * U + cy[k] * V; // c k*u
	U2 = U*U + V*V; // u*u;
	return w[k] * RHO*(1.0 + 3.0*cu + 4.5*cu*cu - 1.5*U2);
}
//=========================================================
//=========================================================
//---------------------------------------------------------
// Subroutine: BGK collision
//---------------------------------------------------------
void Coll_BGK()
{
	int j, i, k;
	double FEQ;
	for (j = 0; j <= Ny; j++) for (i = 0; i <= Nx; i++) for (k = 0; k<Q; k++)

	{
		FEQ = feq(rho[j][i], ux[j][i], uy[j][i], k); // EDF
		f_post[j][i][k] = f[j][i][k] - (f[j][i][k] - FEQ) / tau;
		// Post-collision DFs
	}
}
//=========================================================
//=========================================================
//---------------------------------------------------------
// Subroutine: MRT collision
//---------------------------------------------------------
void Coll_MRT()
{
	int j, i, k;
	double MEQ;
	double m[Q];
	for (j = 0; j <= Ny; j++) for (i = 0; i <= Nx; i++)
	{
		// Transformation from velocity space to moment space:
		m[0] = f[j][i][0] + f[j][i][1] + f[j][i][2] + f[j][i][3] + f[j][i]
			[4] + f[j][i][5] + f[j][i][6] + f[j][i][7] + f[j][i][8];
		m[1] = -4 * f[j][i][0] - f[j][i][1] - f[j][i][2] - f[j][i][3] - f[j]
			[i][4] + 2 * (f[j][i][5] + f[j][i][6] + f[j][i][7] + f[j][i
			][8]);
		m[2] = 4 * f[j][i][0] - 2 * (f[j][i][1] + f[j][i][2] + f[j][i][3] +
			f[j][i][4]) + f[j][i][5] + f[j][i][6] + f[j][i][7] + f[j]
			[i][8];
		m[3] = f[j][i][1] - f[j][i][3] + f[j][i][5] - f[j][i][6] -
			f[j][i][7] + f[j][i][8];
		m[4] = -2 * (f[j][i][1] - f[j][i][3]) + f[j][i][5] - f[j][i][6] -
			f[j][i][7] + f[j][i][8];
		m[5] = f[j][i][2] - f[j][i][4] + f[j][i][5] + f[j][i][6] -
			f[j][i][7] - f[j][i][8];
		m[6] = -2 * (f[j][i][2] - f[j][i][4]) + f[j][i][5] + f[j][i][6] -
			f[j][i][7] - f[j][i][8];
		m[7] = f[j][i][1] - f[j][i][2] + f[j][i][3] - f[j][i][4];
		m[8] = f[j][i][5] - f[j][i][6] + f[j][i][7] - f[j][i][8];
		// Relaxation in moment space:
		for (k = 0; k<Q; k++)
		{
			MEQ = meq(rho[j][i], ux[j][i], uy[j][i], k);
			m[k] = m[k] - s[k] * (m[k] - MEQ); // relaxation
			m[k] /= D[k]; // rescaling
		}
		// Transforming back to the velocity space:
		f_post[j][i][0] = m[0] - 4 * (m[1] - m[2]);
		f_post[j][i][1] = m[0] - m[1] - 2 * (m[2] + m[4]) + m[3] + m[7];

		f_post[j][i][2] = m[0] - m[1] - 2 * (m[2] + m[6]) + m[5] - m[7];
		f_post[j][i][3] = m[0] - m[1] - 2 * (m[2] - m[4]) - m[3] + m[7];
		f_post[j][i][4] = m[0] - m[1] - 2 * (m[2] - m[6]) - m[5] - m[7];
		f_post[j][i][5] = m[0] + m[1] + m[1] + m[2] + m[3] + m[4] + m[5] + m[6]
			+ m[8];
		f_post[j][i][6] = m[0] + m[1] + m[1] + m[2] - m[3] - m[4] + m[5] + m[6]
			- m[8];
		f_post[j][i][7] = m[0] + m[1] + m[1] + m[2] - m[3] - m[4] - m[5] - m[6]
			+ m[8];
		f_post[j][i][8] = m[0] + m[1] + m[1] + m[2] + m[3] + m[4] - m[5] - m[6]
			- m[8];
	}
}
//=========================================================
//=========================================================
//---------------------------------------------------------
// Subroutine: calculation the equilibrium moment
//---------------------------------------------------------
double meq(double RHO, double U, double V, int k)
{
	double x;
	switch (k)
	{
	case 0: {x = RHO; break; }
	case 1: {x = RHO*(-2 + 3 * (U*U + V*V)); break; }
	case 2: {x = RHO*(1 - 3 * (U*U + V*V)); break; }
	case 3: {x = RHO*U; break; }
	case 4: {x = -RHO*U; break; }
	case 5: {x = RHO*V; break; }
	case 6: {x = -RHO*V; break; }
	case 7: {x = RHO*(U*U - V*V); break; }
	case 8: {x = RHO*U*V; break; }
	default: x = 0;
	}
	return x;
}
//=========================================================
//=========================================================
//---------------------------------------------------------
// Subroutine: Streaming
//---------------------------------------------------------
void Streaming()
{
	int j, i, jd, id, k;
	for (j = 0; j <= Ny; j++) for (i = 0; i <= Nx; i++) for (k = 0; k<Q; k++)
	{
		jd = j - cy[k]; id = i - cx[k]; // upwind node
		if (jd >= 0 && jd <= Ny && id >= 0 && id <= Nx) // fluid node
			f[j][i][k] = f_post[jd][id][k]; // streaming
	}
}
//=========================================================
//=========================================================
//---------------------------------------------------------
// Subroutine: Bounce-back scheme
//---------------------------------------------------------
void Bounce_back()
{
	int i, j;
	// j=Ny: top plate
	for (i = 0; i <= Nx; i++)
	{
		f[Ny][i][4] = f_post[Ny][i][2];
		f[Ny][i][7] = f_post[Ny][i][5];
		f[Ny][i][8] = f_post[Ny][i][6];
	}
	// j=0: bottom plate
	for (i = 0; i <= Nx; i++)
	{
		f[0][i][2] = f_post[0][i][4];
		f[0][i][5] = f_post[0][i][7];
		f[0][i][6] = f_post[0][i][8];
	}
	// i=0: left wall
	for (j = 0; j <= 0.4* Ny; j++)
	{
		f[j][0][1] = f_post[j][0][3];
		f[j][0][5] = f_post[j][0][7];
		f[j][0][8] = f_post[j][0][6];
	}
	for (j = 0.6*Ny; j <= Ny; j++)
	{
		f[j][0][1] = f_post[j][0][3];
		f[j][0][5] = f_post[j][0][7];
		f[j][0][8] = f_post[j][0][6];
	}
	for (j = 0.4*Ny; j <= 0.6* Ny; j++)
	{
		ux[j][i] = uuu;
		for (int k = 0; k<9; k++)
			f[j][0][k] = f_post[j][0][k];
		//	f[j][i][k] = feq(rho[j][i], ux[j][i], uy[j][i], k);
	}
	// i=Nx: right wall
	/*	for (j = 0; j <= Ny; j++)
	{
	f[j][Nx][3] = f_post[j][Nx][1];
	f[j][Nx][7] = f_post[j][Nx][5];
	f[j][Nx][6] = f_post[j][Nx][8];
	}*/
}

void zou_he()
{
	int i, j;
	for (j = 0; j <= Ny; j++)
	{
		ux[j][Nx] = 1 - (f_post[j][Nx][0] + f_post[j][Nx][2] + f_post[j][Nx][4] + 2 * (f_post[j][Nx][1] + f_post[j][Nx][5] + f_post[j][Nx][8])) / 1;
		f_post[j][Nx][6] = f_post[j][Nx][8] + 1 / 2 * (f_post[j][Nx][4] - f_post[j][Nx][2]) - 1 / 6 * 1 * ux[j][Nx];
		f_post[j][Nx][7] = f_post[j][Nx][5] - 1 / 2 * (f_post[j][Nx][4] - f_post[j][Nx][2]) - 1 / 6 * 1 * ux[j][Nx];
		f_post[j][Nx][3] = f_post[j][Nx][1] - 2 / 3 * 1 * ux[j][Nx] * 1.3;
	}

	for (j = 0.4*Ny; j <= 0.6* Ny; j++)
	{
		rho[j][0] = 1 / (1 - uuu)*(f_post[j][0][0] + f_post[j][0][2] + f_post[j][0][4] + 2 * (f_post[j][0][3] + f_post[j][0][6] + f_post[j][0][7]));
		f_post[j][0][1] = f_post[j][0][3] + 2 / 3 * rho[j][0] * uuu;
		f_post[j][0][5] = f_post[j][0][7] + 1 / 2 * (f_post[j][0][4] - f_post[j][0][2]) + 1 / 6 * rho[j][0] * uuu;
		f_post[j][0][8] = f_post[j][0][6] + 1 / 2 * (f_post[j][0][2] - f_post[j][0][4]) + 1 / 6 * rho[j][0] * uuu;
	}
}
//========================================================= 
//=========================================================
//------------------------------------------------------------
// Subroutine: Fluid variables (density and velocity)
//------------------------------------------------------------
void Den_Vel()
{
	int j, i;
	for (j = 0; j <= Ny; j++) for (i = 0; i <= Nx; i++)
	{
		rho[j][i] = f[j][i][0] + f[j][i][1] + f[j][i][2] + f[j][i][3]
			+ f[j][i][4] + f[j][i][5] + f[j][i][6] + f[j][i][7] +
			f[j][i][8];
		ux[j][i] = (f[j][i][1] + f[j][i][5] + f[j][i][8] - f[j][i][3] -
			f[j][i][6] - f[j][i][7]) / rho[j][i];
		uy[j][i] = (f[j][i][5] + f[j][i][6] + f[j][i][2] - f[j][i][7] -
			f[j][i][8] - f[j][i][4]) / rho[j][i];
	}
}
//=========================================================
double Err() // Calculating the relative difference in velocity between two steps
{
	int j, i;
	double e1, e2;
	e1 = e2 = 0.0;
	for (j = 1; j<Ny; j++) for (i = 0; i<Nx; i++)
	{
		e1 += sqrt((ux[j][i] - u0[j][i])*(ux[j][i] - u0[j][i])
			+ (uy[j][i] - v0[j][i])*(uy[j][i] - v0[j][i]));
		e2 += sqrt(ux[j][i] * ux[j][i] + uy[j][i] * uy[j][i]);
		u0[j][i] = ux[j][i]; v0[j][i] = uy[j][i];
	}
	return e1 / e2;
}


//void Data_Output() // Output data
/*{
int i, j;
FILE *fp;
char
fp = fopen_s("x.dat", "w+");
for (i = 0; i <= Nx; i++) fprintf(fp, "%e \n", (i + 0.5) / L);
fclose(fp);
fp = fopen_s("y.dat", "w+");
for (j = 0; j <= Ny; j++) fprintf(fp, "%e \n", (j + 0.5) / L);
fclose(fp);
fp = fopen("ux.dat", "w");
for (j = 0; j <= Ny; j++) {
for (i = 0; i <= Nx; i++) fprintf(fp, "%e ", ux[j][i]);
fprintf(fp, "\n");
}
fclose(fp);
fp = fopen("uy.dat", "w");
for (j = 0; j <= Ny; j++) {
for (i = 0; i <= Nx; i++) fprintf(fp, "%e ", uy[j][i]);
fprintf(fp, "\n");
}
fclose(fp);
fp = fopen("rho.dat", "w");
for (j = 0; j <= Ny; j++) {
for (i = 0; i <= Nx; i++) fprintf(fp, "%e ", rho[j][i]);
fprintf(fp, "\n");
}
fclose(fp);
}*/


void outputdata(int n)
{
	if (n % 50 == 0)
	{
		ofstream file;
		char temp[10];
		string str;
		n = n / 50;
		//_itoa_s(n, temp, 10);
		//str = string(temp);
		//str += "uv.ppm";
		// str += "uv1.ppm";

		char ss[10];

		sprintf_s(ss, "%04d", n);
		str = string(ss);
		str += ".ppm";

		file.open(str.c_str());
		float maxVelocity = 0;
		float minVelocity = 999;
		if (file.is_open())
		{
			file << "P3" << endl;
			file << 2 * Nx - 16 << " " << 2 * Ny - 2 << endl;
			file << "255" << endl;


			//#pragma omp parallel for
			for (int j = 0; j < Ny; j++)
				for (int i = 0; i < Nx; i++)
				{
					float uu = sqrt(pow(ux[j][i], 2) + pow(uy[j][i], 2));
			//		Grid::vec3 result = mapVelocityToColor(uu);

					{
						a[i][j][1] = ceil(255 * mapVelocityToColor(uu).x);
						a[i][j][2] = ceil(255 * mapVelocityToColor(uu).y);
						a[i][j][3] = ceil(255 * mapVelocityToColor(uu).z); }
				}


			//#pragma omp parallel for
			for (int j = 0; j < Ny - 1; j++)
				for (int i = 0; i < Nx - 1; i++)
					for (int k = 1; k <= 3; k++)
					{//pixel interplation
						b[2 * i][2 * j][k] = 0.75*a[i][j][k] + 0.25*a[i + 1][j + 1][k];
						b[2 * i + 1][2 * j + 1][k] = 0.25*a[i][j][k] + 0.75*a[i + 1][j + 1][k];
						b[2 * i + 1][2 * j][k] = 0.75*a[i + 1][j][k] + 0.25*a[i][j + 1][k];
						b[2 * i][2 * j + 1][k] = 0.25*a[i + 1][j][k] + 0.75*a[i][j + 1][k];
					}
			//cout << omp_get_num_threads() << ":" << omp_get_max_threads() << endl;
			//#pragma omp parallel for
			for (int j = 0; j <2 * Ny - 2; j++)
				for (int i = 0; i < 2 * Nx - 16; i++)
				{
					if (i < 2 * Nx - 17)
						file << b[i][j][1] << "  " << b[i][j][2] << " " << b[i][j][3] << "  ";
					else
						file << b[i][j][1] << " " << b[i][j][2] << " " << b[i][j][3] << endl;
				}
			//	int finish_time;
			//	int a = finish_time;
			//	finish_time = clock();
			//	cout << "time for 50 timesteps is " << (finish_time - a) / 1000 << endl;
			//	cout << "total time is " << (finish_time - begin_time) / 1000 << "s" << endl;
			//out put the RGB before the pixel interplation
			/*	for (int j = 0; j < NY; j++)
			for (int i = 0; i < NX; i++)
			if (i <  NX - 1)
			file << a[i][j][1] << "," << a[i][j][2] << "," << a[i][j][3] << ",,";
			else

			file << a[i][j][1] << "," << a[i][j][2] << "," << a[i][j][3] << endl;*/

			//out put the velocity

			/*for (int j = 0; j < 2 * NY; j++)
			for (int i = 0; i < 2 * NX; i++)
			if (i < 2 * NX - 1)
			//	file << u[i][j][0] << "," <<u[i][j][1] << ",";
			file << sqrt(pow(u[i][j][0], 2) + pow(u[i][j][1], 2)) << ",";
			else
			//	file << u[i][j][0] << "," << u[i][j][1] << ",";
			file << sqrt(pow(u[i][j][0], 2) + pow(u[i][j][1], 2)) << endl;*/
		}
	}
}


