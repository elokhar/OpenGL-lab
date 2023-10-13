#include <iostream>

#define GL3_PROTOTYPES 1
#include <glew.h>
#include <SDL.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#define WIDTH 800
#define HEIGHT 600
#define vao_size 2
#define vbo_size 5

//------------------------------------------------------------------------------------------------------------------------------
//Globalne zmienne 

// kod zrodlowy shadera wierzcholkow
const GLchar* vertexSource =
"#version 150 core\n"
"in vec3 position;"
"in vec3 color;"
"in vec2 texcoord;"
"out vec3 Color;"
"out vec2 Texcoord;"
"uniform mat4 transformMatrix;"
"void main()"
"{"
"    gl_Position =  transformMatrix * vec4(position, 1.0);"
"    Color = color;"
"	 Texcoord = texcoord;"
"}";

// kod zrodlowy shadera fragmentow
const GLchar* fragmentSource =
"#version 150 core\n"
"in vec3 Color;"
"in vec2 Texcoord;"
"out vec4 outColor;"
"uniform sampler2D tex;"
"void main()"
"{"
"    outColor = texture(tex, Texcoord) * vec4(Color, 1.0);"
"}";


//------------------------------------------------------------------------------------------------------------------------------

GLint posAttrib, colAttrib, texAttrib;					//wskazniki atrybutow wierzcholkow
GLuint vertexShader, fragmentShader, shaderProgram;		//shadery


GLuint vao[vao_size], vbo[vbo_size], ebo, tex;	// identyfikatory poszczegolnych obiektow (obiekty tablic wierzcholkow, buforow wierzcholkow, elementow, tekstury)

//------------------------------------------------------------------------------------------------------------------------------

GLfloat ver_floor[] = { //wspolrzedne wierzcholkow podlogi
	-5.0f,  0.0f, -5.0f,
	5.0f,  0.0f, -5.0f,
	5.0f,  0.0f,  5.0f,
	-5.0f,  0.0f,  5.0f,
};

GLfloat col_floor[] = { //kolory wierzcholkow podlogi
	0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,
};

GLfloat ver_wall[] = { //wspolrzedne wierzcholkow sciany
	-1.0f,  0.0f, 0.0f,
	-1.0f,  2.0f, 0.0f,
	1.0f,  2.0f, 0.0f,
	1.0f,  0.0f, 0.0f,
};

GLfloat col_wall[] = { //kolory wierzcholkow sciany
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
};

GLfloat tex_wall[] = {	//wspolrzedne tekstury dla sciany, 
						//jesli wykraczaja poza przedzial <0,1> nastapi nakladanie zalezne od parametrow ustawionych za pomoca funkcji glTexParameteri, np. GL_REPEAT
	0.0f, 0.0f,	// lewy dolny rog tekstury
	5.0f, 0.0f,
	5.0f, 5.0f,
	0.0f, 5.0f, // prawy gorny rog tekstury
};

GLuint elements[] = { // pogrupowanie wierzcholkow w trojkaty, wykorzystane zarowno dla sciany jaki dla podlogi
	0,1,2,
	2,3,0,
};

float pixels[] = {			//tekstura o wymiarach 4x4; dla kazdego punktu okreslone skladowe RGB koloru 
	0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,

};

//------------------------------------------------------------------------------------------------------------------------------

void init_shaders()
{
	// tworzenie i kompilacja shadera wierzcholkow
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	// tworzenie i kompilacja shadera fragmentow
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	// dolaczenie programow przetwarzajacych wierzcholki i fragmenty do programu cieniujacego
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);
	
	// wskazniki atrybutow wierzcholkow
	posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);

}

//------------------------------------------------------------------------------------------------------------------------------

void create_objects()
{
	// generowanie obiektow
	glGenVertexArrays(vao_size, vao);  // obiekt tablicy wierzcholkow, dla kazdego obiektu (sciana, podloga) mamy jedna tablice
	glGenBuffers(vbo_size, vbo);		// obiekty buforow wierzcholkow, dla kazdego typu atrubutow kazdego obiektu mamy jeden bufor (np. bufor dla kolorow sciany, bufor dla wspolrzednych podlogi itd.)
	glGenBuffers(1, &ebo);		// obiekt bufora elementow (ten sam bufor wukorzystany dla sciany i podlogi)
	glGenTextures(1, &tex);		// obiekt tekstury

	// sciana vao[0]

	glBindVertexArray(vao[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);	// bufor wspolrzednych wierzcholkow sciany
	glBufferData(GL_ARRAY_BUFFER, sizeof(ver_wall), ver_wall, GL_STATIC_DRAW);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);	// bufor kolorow wierzcholkow sciany
	glBufferData(GL_ARRAY_BUFFER, sizeof(col_wall), col_wall, GL_STATIC_DRAW);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);	// bufor wspolrzednych tekstury sciany
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_wall), tex_wall, GL_STATIC_DRAW);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texAttrib);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	// podloga vao[1]

	glBindVertexArray(vao[1]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);	// bufor wspolrzednych wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(ver_floor), ver_floor, GL_STATIC_DRAW);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);	// bufor kolorow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(col_floor), col_floor, GL_STATIC_DRAW);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colAttrib);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
}

//------------------------------------------------------------------------------------------------------------------------------

void configure_texture()
{
	glBindTexture(GL_TEXTURE_2D, tex);		// powiazanie tekstury z obiektem (wybor tekstury)

	// ustawienia parametrow tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// sposob nakladania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sposob filtrowania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_FLOAT, pixels); // ladowanie do tekstury tablicy pikseli
}

//------------------------------------------------------------------------------------------------------------------------------

int main(int argc, char ** argv)
{	
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_Window* window = SDL_CreateWindow("OpenGL", 100, 100, 800, 600, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_Event windowEvent;

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cout << "ERROR" << std::endl;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); // GL_ALWAYS)
	
	
	init_shaders();
				
	create_objects();

	configure_texture();
			

	glm::mat4 viewMatrix = glm::lookAt(glm::vec3(0.0f, 5.0f, 10.0f), glm::vec3(0.0f, 1.0f, -5.0f), glm::vec3(0.0f, 1.0f, 0.0f));  //macierz widoku
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 20.0f);		 //macierz rzutowania perspektywicznego
	glm::mat4 translationMatrix;
	glm::mat4 rotationMatrix;
	glm::mat4 transformMatrix;

	GLint transformMatrixUniformLocation = glGetUniformLocation(shaderProgram, "transformMatrix");

	GLfloat angle = 0;

	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_KEYUP &&
				windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
			if (windowEvent.type == SDL_QUIT) break;

		}
	
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);	// szare tlo
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		translationMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));						// macierz przesuniecia o zadany wektor
		rotationMatrix = glm::rotate(glm::mat4(), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));		// macierz obrotu o zadany kat wokol osi y
		transformMatrix = projectionMatrix * viewMatrix * translationMatrix * rotationMatrix;				// wynikowa macierz transformacji
		glUniformMatrix4fv(transformMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(transformMatrix));	// macierz jako wejœciowa zmienna dla shadera wierzcholkow
								
		glBindVertexArray(vao[1]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);	//rysujemy podloge

		glBindVertexArray(vao[0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);	//rysujemy sciane

		angle += -0.05;
		SDL_GL_SwapWindow(window);
	}

	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(5, vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteTextures(1, &tex);
	glDeleteVertexArrays(2, vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();

	return 0;
}