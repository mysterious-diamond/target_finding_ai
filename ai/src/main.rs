use bytemuck::cast_slice;
use rand_distr::{Distribution, Normal};
use std::{error::Error, io};
use tokio::net::windows::named_pipe::{NamedPipeServer, ServerOptions};

struct AI {
    first_layer_weights: Vec<f32>,
    first_layer_biases: Vec<f32>,
    second_layer_weights: Vec<f32>,
    second_layer_biases: Vec<f32>,
    output_layer_weights: Vec<f32>,
    output_layer_biases: Vec<f32>,
    hidden_layer_neurons: usize,
    standard_deviation: f32,
    learning_rate: f32,
}

impl AI {
    pub fn new() -> AI {
        let hidden_layer_neurons: usize = 32;
        let mut first_layer_weights: Vec<f32> = vec![0.0; 2 * hidden_layer_neurons];
        let mut second_layer_weights: Vec<f32> =
            vec![0.0; hidden_layer_neurons * hidden_layer_neurons];
        let mut output_layer_weights: Vec<f32> = vec![0.0; hidden_layer_neurons * 2];

        let first_layer_biases: Vec<f32> = vec![0.0; hidden_layer_neurons];
        let second_layer_biases: Vec<f32> = vec![0.0; hidden_layer_neurons];
        let output_layer_biases: Vec<f32> = vec![0.0; 2];

        let first_layer_distribution = Normal::new(0.0, 1.0f32.sqrt()).unwrap();
        let second_layer_distribution =
            Normal::new(0.0, (2.0 / hidden_layer_neurons as f32).sqrt()).unwrap();
        let output_layer_distribution =
            Normal::new(0.0, (1.0 / hidden_layer_neurons as f32).sqrt()).unwrap();

        for i in 0..hidden_layer_neurons {
            for j in 0..2 {
                first_layer_weights[i * 2 + j] = first_layer_distribution.sample(&mut rand::rng());
            }
        }

        for i in 0..hidden_layer_neurons {
            for j in 0..hidden_layer_neurons {
                second_layer_weights[i * hidden_layer_neurons + j] =
                    second_layer_distribution.sample(&mut rand::rng());
            }
        }

        for i in 0..2 {
            for j in 0..hidden_layer_neurons {
                output_layer_weights[i * hidden_layer_neurons + j] =
                    output_layer_distribution.sample(&mut rand::rng());
            }
        }

        AI {
            first_layer_weights,
            first_layer_biases,
            second_layer_weights,
            second_layer_biases,
            output_layer_weights,
            output_layer_biases,
            hidden_layer_neurons,
            standard_deviation: 0.5,
            learning_rate: 0.005,
        }
    }

    pub fn forward(&self, inputs: &[f32]) -> (Vec<f32>, Vec<f32>, Vec<f32>, Vec<f32>) {
        let mut first_layer_activations: Vec<f32> = vec![0.0; self.hidden_layer_neurons];

        for (i, bias) in self.first_layer_biases.iter().enumerate() {
            let mut sum: f32 = *bias;
            for (j, input) in inputs.iter().enumerate() {
                sum += self.first_layer_weights[i * 2 + j] * input;
            }

            first_layer_activations[i] = if sum > 0.0 { sum } else { 0.0 };
        }

        let mut second_layer_activations: Vec<f32> = vec![0.0; self.hidden_layer_neurons];

        for (i, bias) in self.second_layer_biases.iter().enumerate() {
            let mut sum: f32 = *bias;
            for (j, activation) in first_layer_activations.iter().enumerate() {
                sum += self.second_layer_weights[i * self.hidden_layer_neurons + j] * activation;
            }

            second_layer_activations[i] = if sum > 0.0 { sum } else { 0.0 };
        }

        let mut output_layer_activations: Vec<f32> = vec![0.0; 2];
        for (i, bias) in self.output_layer_biases.iter().enumerate() {
            let mut sum: f32 = *bias;
            for (j, activation) in second_layer_activations.iter().enumerate() {
                sum += self.output_layer_weights[i * self.hidden_layer_neurons + j] * activation;
            }

            output_layer_activations[i] = sum.tanh();
        }

        let mut outputs: Vec<f32> = vec![0.0; 2];
        for (i, output) in outputs.iter_mut().enumerate() {
            let distribution =
                Normal::new(output_layer_activations[i], self.standard_deviation).unwrap();
            *output = distribution.sample(&mut rand::rng());
        }

        (
            first_layer_activations,
            second_layer_activations,
            output_layer_activations,
            outputs,
        )
    }

    pub fn train(
        &mut self,
        first_layer_activations: &[f32],
        second_layer_activations: &[f32],
        output_layer_activations: &[f32],
        inputs: &[f32],
        outputs: &[f32],
        score: f32,
    ) {
        let mut second_layer_weights: Vec<f32> = vec![0.0; self.hidden_layer_neurons.pow(2)];
        let mut second_layer_biases: Vec<f32> = vec![0.0; self.hidden_layer_neurons];

        let mut output_layer_weights: Vec<f32> = vec![0.0; self.hidden_layer_neurons * 2];
        let mut output_layer_biases: Vec<f32> = vec![0.0; 2];

        let mut output_layer_loss: Vec<f32> = vec![0.0; 2];
        for i in 0..2 {
            output_layer_loss[i] = (-(outputs[i] - output_layer_activations[i]) * score
                / self.standard_deviation.powf(2.0))
            .clamp(-5.0, 5.0);

            output_layer_loss[i] *= 1.0 - output_layer_activations[i].powf(2.0);
        }

        for (i, loss) in output_layer_loss.iter().enumerate() {
            for j in 0..self.hidden_layer_neurons {
                output_layer_weights[i * self.hidden_layer_neurons + j] = self.output_layer_weights
                    [i * self.hidden_layer_neurons + j]
                    - loss * second_layer_activations[j] * self.learning_rate;
            }

            output_layer_biases[i] = self.output_layer_biases[i] - loss * self.learning_rate;
        }

        let mut second_layer_loss: Vec<f32> = vec![0.0; self.hidden_layer_neurons];
        for (j, loss) in second_layer_loss.iter_mut().enumerate() {
            for (i, output_loss) in output_layer_loss.iter().enumerate() {
                *loss += self.output_layer_weights[i * self.hidden_layer_neurons + j] * output_loss;
            }

            *loss *= if second_layer_activations[j] > 0.0 {
                1.0
            } else {
                0.0
            };
        }

        for (i, loss) in second_layer_loss.iter().enumerate() {
            for j in 0..self.hidden_layer_neurons {
                second_layer_weights[i * self.hidden_layer_neurons + j] = self.second_layer_weights
                    [i * self.hidden_layer_neurons + j]
                    - loss * first_layer_activations[j] * self.learning_rate;
            }
            second_layer_biases[i] = self.second_layer_biases[i] - loss * self.learning_rate;
        }

        let mut first_layer_loss: Vec<f32> = vec![0.0; self.hidden_layer_neurons];
        for (j, loss) in first_layer_loss.iter_mut().enumerate() {
            for (i, second_loss) in second_layer_loss.iter().enumerate() {
                *loss += self.second_layer_weights[i * self.hidden_layer_neurons + j] * second_loss;
            }

            *loss *= if first_layer_activations[j] > 0.0 {
                1.0
            } else {
                0.0
            };
        }

        for (i, loss) in first_layer_loss.iter().enumerate() {
            for (j, input) in inputs.iter().enumerate() {
                self.first_layer_weights[i * 2 + j] -= loss * input * self.learning_rate;
            }

            self.first_layer_biases[i] -= loss * self.learning_rate;
        }

        self.second_layer_weights = second_layer_weights;
        self.second_layer_biases = second_layer_biases;
        self.output_layer_weights = output_layer_weights;
        self.output_layer_biases = output_layer_biases;
        if score == 10000.0 {
            self.standard_deviation *= 0.9995;
        }
        self.standard_deviation = self.standard_deviation.max(0.2);
    }
}

async fn await_pipe_connection() -> Result<NamedPipeServer, Box<dyn Error>> {
    let pipe: NamedPipeServer = ServerOptions::new().create(r"\\.\pipe\target-ai")?;

    println!("Waiting for connection from interface...");
    pipe.connect().await?;

    println!("Pipe connected succesfully. Awaiting input...");
    Ok(pipe)
}

async fn write_data(pipe: &NamedPipeServer, data: &[f32]) {
    loop {
        pipe.writable()
            .await
            .expect("Error while waiting for pipe to be readable");

        match pipe.try_write(cast_slice(data)) {
            Ok(_) => break,
            Err(err) if err.kind() == io::ErrorKind::WouldBlock => continue,
            Err(err) => panic!("Error while trying to write, {}", err),
        }
    }
}

async fn wait_for_score_message(pipe: &NamedPipeServer) -> f32 {
    let mut buf = vec![0; 4];
    loop {
        pipe.readable()
            .await
            .expect("Error while waiting for score message");

        match pipe.try_read(&mut buf) {
            Ok(0) => return 0.0,
            Ok(n) => {
                let data: Vec<f32> = buf[..n]
                    .chunks_exact(4)
                    .map(|chunk| {
                        let array: [u8; 4] = chunk.try_into().expect("Data broken or corrupted");
                        f32::from_le_bytes(array)
                    })
                    .collect();

                return data[0];
            }
            Err(err) if err.kind() == io::ErrorKind::WouldBlock => continue,
            Err(err) => panic!("Error while reading score {}", err),
        }
    }
}

async fn listener_loop(pipe: &NamedPipeServer, mut ai: AI, mut buffer: Vec<u8>) {
    loop {
        pipe.readable()
            .await
            .expect("Error while waiting for message");

        match pipe.try_read(&mut buffer) {
            Ok(0) => break,
            Ok(n) => {
                let data: Vec<f32> = buffer[..n]
                    .chunks_exact(4)
                    .map(|chunk| {
                        let array: [u8; 4] = chunk.try_into().expect("Data broken or corrupted");
                        f32::from_le_bytes(array)
                    })
                    .collect();
                let (
                    first_layer_activations,
                    second_layer_activations,
                    output_layer_activations,
                    outputs,
                ) = ai.forward(&data);
                write_data(pipe, &outputs).await;
                let score: f32 = wait_for_score_message(pipe).await;
                ai.train(
                    &first_layer_activations,
                    &second_layer_activations,
                    &output_layer_activations,
                    &data,
                    &outputs,
                    score,
                );
            }
            Err(err) if err.kind() == std::io::ErrorKind::WouldBlock => continue,
            Err(err) if err.kind() == std::io::ErrorKind::BrokenPipe => break,
            Err(err) => {
                panic!(
                    "Error while trying to read message, error message \n{}",
                    err
                );
            }
        }
    }
}

#[tokio::main]
async fn main() {
    let pipe: NamedPipeServer = await_pipe_connection()
        .await
        .expect("Couldn't connect pipe...");

    let ai: AI = AI::new();

    let buffer = vec![0; 2 * 4];
    listener_loop(&pipe, ai, buffer).await;
}
