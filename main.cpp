#include <iostream>

#include <SFML/Graphics.hpp>
#include "fftInput.h"
#include "micInput.h"
#include "fileInput.h"
#include <math.h>
#include <thread>

sf::Color HStoRGB(float H, float S)
{
    S = std::clamp(S, 0.f, 1.f);
	float HPrime = std::fmod(H / (M_PIf / 3), 6.f); // H'
	float X = S * (1 - std::fabs(std::fmod(HPrime, 2.f) - 1));
	float M = 1 - S;

	float R = 0.f;
	float G = 0.f;
	float B = 0.f;

	switch (static_cast<int>(HPrime))
	{
	case 0:        G = X; B = S; break; // [3, 4)
	case 1:        G = S; B = X; break; // [2, 3)
	case 2: R = X; G = S;        break; // [1, 2)
	case 3: R = S; G = X;        break; // [0, 1)
	case 4: R = S;        B = X; break; // [5, 6)
	case 5: R = X;        B = S; break; // [4, 5)
	}

	R += M;
	G += M;
	B += M;

	sf::Color color;
	color.r = std::round<uint8_t>(R * 255);
	color.g = std::round<uint8_t>(G * 255);
	color.b = std::round<uint8_t>(B * 255);

	return color;
}

int main(int argc, char *argv[]) {
    std::cout << "Starting SFML" << std::endl;

    sf::RenderWindow window(sf::VideoMode({800, 800}), argv[0]);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::unique_ptr<FFTInput> fftInput;
    if (argc > 1) {
        std::cout << "Using file input: " << argv[1] << std::endl;
        auto music = std::make_unique<sf::Music>(argv[1]);
        fftInput = std::make_unique<FileInput>(std::move(music));
    } else {
        std::cout << "Using microphone input" << std::endl;
        fftInput = std::make_unique<MicInput>();
    }
    window.setView(sf::View{{0, 0}, {10, 10}});
    window.setFramerateLimit(100);
    float bassAccumulator = 0;
    while (window.isOpen()) {
        std::optional<sf::Event> event;
        while (event = window.pollEvent()) {

            if (event->is<sf::Event::Closed>()) {
                window.close();
                continue;
            }
            if  (event->is<sf::Event::Resized>()) {
                sf::View view = window.getView();
                float aspectRatio = static_cast<float>(event->getIf<sf::Event::Resized>()->size.x) / event->getIf<sf::Event::Resized>()->size.y;
                if (aspectRatio > 1) {
                    view.setSize({10 * aspectRatio, 10});
                } else {
                    view.setSize({10, 10 / aspectRatio});
                }
                window.setView(view);
                window.display();
                continue;
            }
            fftInput->handleEvent(*event);
        }

        auto fftData = fftInput->getFFT();
        if(fftData.has_value())
        {
            std::array<float, DFT_SIZE> mag;
            for (size_t i = 0; i < mag.size(); ++i) {
                mag[i] = std::log(std::abs((*fftData)[i]) + 1);
            }

            bassAccumulator += (M_PI_2f-std::fmod(bassAccumulator + M_PI_2f, M_PIf)) * 0.2f;
            for (size_t i = 0; i < DFT_SIZE/32; i++)
            {
                bassAccumulator += std::max(mag[i] - 2.f, 0.f) * 0.05f;
            }
            float tilt = sin(bassAccumulator) * M_PI/100;
            

            window.clear();
            sf::VertexArray vertexArray(sf::PrimitiveType::TriangleFan, DFT_SIZE + 2);
            vertexArray[0].position = sf::Vector2f(0, 0);
            for (size_t i = 1; i < DFT_SIZE + 2; ++i) {
                size_t i2 = (i - 1)  % DFT_SIZE;
                float graphValue = mag[i2];
                float x = sin(2 * M_PIf * i2 / DFT_SIZE + tilt) * (graphValue+1.5);
                float y = cos(2 * M_PIf * i2 / DFT_SIZE + tilt) * (graphValue+1.5);
                vertexArray[i].position = sf::Vector2f(x, y);
                vertexArray[i].color = HStoRGB(std::clamp(graphValue*4, 0.f, 2*M_PIf), std::sqrt(graphValue));
            }
            window.draw(vertexArray);
            window.display();
        }
    }

    return 0;
}