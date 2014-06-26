#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/*
/ # cat /proc/mdstat
Personalities : [raid0] [raid1] [raid5]
read_ahead 1024 sectors
md0 : active raid0 sda[0] sdd[3] sdc[2] sdb[1]
      250090496 blocks 64k chunks

unused devices: <none>


/ # cat /proc/mdstat
Personalities : [raid0] [raid1] [raid5]
read_ahead 1024 sectors
md0 : active raid5 sda[0] sdd[3] sdc[2] sdb[1]
      187567872 blocks level 5, 64k chunk, algorithm 2 [4/4] [UUUU]
      [=>...................]  resync =  8.6% (5430952/62522624) finish=28.9min speed=32835K/sec
unused devices: <none>

*/
#define RAID_MAX_DISKS 8
struct raid_disk {
	int level; /* RAID level: 0, 1, 5 */
	int pdisk_cnt; /* number of physical disks involved */
	char pdisk[RAID_MAX_DISKS][8];
};

int main(void)
{
	int i, dev_cnt = 0, level, idx, ph;
	FILE *mdstat;
	char buf[256], status[16];

	struct raid_disk raid_ = {0};
	struct raid_disk *raid = &raid_;

	mdstat = fopen("file.proc_mdstat", "r");
	if (mdstat == 0) {
		perror("Cannot open 'file.proc_mdstat'\n");
		return -errno;
	}

	do {
		buf[0] = 0;
		while (buf[0] != 'm' || buf[1] != 'd')
			if (fgets(buf, sizeof(buf), mdstat) == 0) {
				fclose(mdstat);
				return 0;
			}
		i = sscanf(buf, "md%d : %s raid%d", &idx, status, &level);
		if (i != 3 || (level != 0 && level != 1 && level != 5) || strcmp(status, "active"))
			continue;
		ph = strstr(buf, "raid") - buf + sizeof("raid") + 1;
		raid->pdisk_cnt = 0;
		while (raid->pdisk_cnt < RAID_MAX_DISKS) {
			while (buf[ph] && isspace(buf[ph]) && ph < sizeof(buf))
				ph++;
			if (buf[ph] == 0 || ph >= sizeof(buf))
				break;
			i = sscanf(buf + ph, "%7[^[][%*d]",
				raid->pdisk[raid->pdisk_cnt]);
			if (i != 1)
				break;
			raid->pdisk_cnt++;
			while (buf[ph] && !isspace(buf[ph]) && ph < sizeof(buf))
				ph++;
			if (buf[ph] == 0 || ph >= sizeof(buf))
				break;
		}
		if (raid->pdisk_cnt == 0)
			continue;
		raid->level = level;
		dev_cnt++;
	} while (dev_cnt < 1);

	fclose(mdstat);
	return 0;
}

