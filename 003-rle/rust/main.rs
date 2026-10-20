// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Oxide Club contributors
use std::env;
use std::io::{self, BufReader, BufWriter, Read, Write};
use std::process::ExitCode;

// EOF é distinto de qualquer byte. Interrupted não representa fim do stream.
fn byte(input: &mut impl Read) -> Result<Option<u8>, String> {
    let mut buffer = [0];
    loop {
        match input.read(&mut buffer) {
            Ok(0) => return Ok(None),
            Ok(_) => return Ok(Some(buffer[0])),
            Err(error) if error.kind() == io::ErrorKind::Interrupted => continue,
            Err(error) => return Err(format!("erro de leitura: {error}")),
        }
    }
}

fn write(output: &mut impl Write, data: &[u8]) -> Result<(), String> {
    output
        .write_all(data)
        .map_err(|error| format!("erro de escrita: {error}"))
}

fn encode(input: &mut impl Read, output: &mut impl Write) -> Result<(), String> {
    let mut value = 0;
    let mut count: u8 = 0;
    while let Some(next) = byte(input)? {
        // Emite antes de incrementar para nunca transbordar a contagem.
        if count != 0 && (next != value || count == 255) {
            write(output, &[count, value])?;
            count = 0;
        }
        value = next;
        count += 1;
    }
    if count != 0 {
        write(output, &[count, value])?;
    }
    Ok(())
}

fn decode(input: &mut impl Read, output: &mut impl Write) -> Result<(), String> {
    while let Some(count) = byte(input)? {
        if count == 0 {
            return Err("contagem zero".into());
        }
        let value = byte(input)?.ok_or("par incompleto")?;
        // O bloco tem tamanho fixo, independente do tamanho descomprimido.
        write(output, &[value; 255][..usize::from(count)])?;
    }
    Ok(())
}

fn run() -> Result<(), String> {
    // args_os permite rejeitar argumentos que não são UTF-8 sem panic.
    let mut args = env::args_os().skip(1);
    let mode = args.next();
    let encoding = match mode.as_deref().and_then(|mode| mode.to_str()) {
        Some("encode") => true,
        Some("decode") => false,
        _ => return Err("uso: oxide-rle encode|decode".into()),
    };
    if args.next().is_some() {
        return Err("uso: oxide-rle encode|decode".into());
    }
    let mut input = BufReader::new(io::stdin().lock());
    let mut output = BufWriter::new(io::stdout().lock());
    let result = if encoding {
        encode(&mut input, &mut output)
    } else {
        decode(&mut input, &mut output)
    };
    // Drop ignora erros de flush; precisamos verificá-los explicitamente.
    output
        .flush()
        .map_err(|error| format!("erro de escrita: {error}"))?;
    result
}

fn main() -> ExitCode {
    match run() {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            let _ = writeln!(io::stderr().lock(), "oxide-rle: {error}");
            ExitCode::FAILURE
        }
    }
}
