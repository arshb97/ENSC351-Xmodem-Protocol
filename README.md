# ENSC351-Xmodem-Protocol Multipart Project

XMODEM is a popular file transfer protocol developed by Ward Christensen in 1977. It sends data blocks associated with checksums and waits for the acknowledgment of a block receipt

XMODEM is a half-duplex communication protocol that has an effective error detection strategy. It breaks the original data into a series of packets, which are sent to the receiver together with additional information that permits the receiver to determine whether packets were properly received.

Files are marked complete with an end-of-file character that is sent after the last block. This character is not in the packet, but is sent as a single byte. Because file length is not passed as part of the protocol, the last packets are padded with known characters, which can be dropped. 

Files are transferred one packet at a time. On the receiving side, the packet checksum is calculated and compared to the one received at the end of the packet. When the receiver sends an acknowledgment message to the sender, the next set of packets is sent. If there is a problem with the checksum, the receiver sends a message requesting retransmission. Upon receiving the negative acknowledgement, the sender resends the packet and retries the transmission continuously for about 10 times before aborting the transfer. 
