#include <iostream>

#include <SFML/Graphics.hpp>
#include "fftInput.h"
#include "micInput.h"
#include "fileInput.h"
#include <filesystem>
#include <math.h>
#include <algorithm>
#include "math_constants.h"
#include <thread>

using namespace std;

sf::Color HStoRGB(float H, float S)
{
    S = clamp(S, 0.f, 1.f);
    float HPrime = fmod(H / (fft::PI / 3.f), 6.f); // H'
	float X = S * (1 - fabs(fmod(HPrime, 2.f) - 1));
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
    color.r = static_cast<uint8_t>(std::round(R * 255));
    color.g = static_cast<uint8_t>(std::round(G * 255));
    color.b = static_cast<uint8_t>(std::round(B * 255));

	return color;
}

int main(int argc, char *argv[]) {
    cout << "Starting SFML" << endl;

    sf::RenderWindow window(sf::VideoMode({800, 800}), argv[0]);

    unique_ptr<FFTInput> fftInput;
    if (argc > 1) {
        using namespace filesystem;
        path filename{argv[1]};
        cout << "Using file input: " << relative(filename) << endl;
        window.setTitle(filename.stem().string());
        fftInput = make_unique<FileInput>(make_unique<sf::Music>(filename));
    } else {
        cout << "Using microphone input" << endl;
        fftInput = make_unique<MicInput>();
    }
    sf::View fftView = {{0, 0}, {10, 10}};
    sf::View barView = window.getDefaultView();
    window.setFramerateLimit(100);
    float bassAccumulator = 0;
    while (window.isOpen()) {
        optional<sf::Event> event;
        while (event = window.pollEvent()) {

            if (event->is<sf::Event::Closed>()) {
                window.close();
                continue;
            }
            if  (auto* resize = event->getIf<sf::Event::Resized>()) {
                float aspectRatio = static_cast<float>(resize->size.x) / resize->size.y;
                if (aspectRatio > 1) {
                    fftView.setSize({10 * aspectRatio, 10});
                } else {
                    fftView.setSize({10, 10 / aspectRatio});
                }
                barView = sf::View{sf::FloatRect{{0, 0}, (sf::Vector2f) resize->size}};
                window.setView(fftView);
                window.display();
                continue;
            }
            fftInput->handleEvent(*event);
        }

        auto fftData = fftInput->getFFT();
        if(fftData.has_value())
        {
            array<float, DFT_SIZE> mag;
            for (size_t i = 0; i < mag.size(); ++i) {
                mag[i] = log(abs((*fftData)[i]) + 1);
            }

            bassAccumulator += (fft::HALF_PI - fmod(bassAccumulator + fft::HALF_PI, fft::PI)) * 0.2f;
            for (size_t i = 0; i < DFT_SIZE/32; i++)
            {
                bassAccumulator += max(mag[i] - 2.f, 0.f) * 0.05f;
            }
            float tilt = sin(bassAccumulator) * fft::PI / 100.f;
            

            sf::VertexArray vertexArray(sf::PrimitiveType::TriangleFan, DFT_SIZE + 2);
            vertexArray[0].position = sf::Vector2f(0, 0);
            for (size_t i = 1; i < DFT_SIZE + 2; ++i) {
                size_t i2 = (i - 1)  % DFT_SIZE;
                float graphValue = mag[i2];
                vertexArray[i].position = sf::Vector2f(graphValue + 1.5, sf::radians(2 * fft::PI * i2 / DFT_SIZE + fft::HALF_PI + tilt));
                vertexArray[i].color = HStoRGB(clamp(graphValue*4, 0.f, 2*fft::PI), sqrt(graphValue));
            }

            sf::Vector2f windowSize = barView.getSize();
            float barHeight = max(10.f, windowSize.y * 0.015f);
            sf::RectangleShape rect({fftInput->getTime() * windowSize.x, barHeight});
            rect.setOrigin(rect.getPoint(3));
            rect.setPosition({0, windowSize.y});
            rect.setFillColor(sf::Color::White);

            window.clear();
            window.setView(barView);
            window.draw(rect);

            window.setView(fftView);
            window.draw(vertexArray);

            window.display();
        }
    }

    return 0;
}