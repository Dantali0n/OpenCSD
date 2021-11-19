template<class nvme_zns_backend>
NvmeZns<nvme_zns_backend>::NvmeZns(nvme_zns_backend* backend) {
    this->backend = backend;
}

template<class nvme_zns_backend>
void NvmeZns<nvme_zns_backend>::get_nvme_zns_info(struct nvme_zns_info* info) {
    this->backend->get_nvme_zns_info(info);
}

template<class nvme_zns_backend>
int NvmeZns<nvme_zns_backend>::read(
    uint64_t zone, uint64_t sector, uint64_t offset, void* buffer, uint64_t size)
{
    return this->backend->read(zone, sector, offset, buffer, size);
}

template<class nvme_zns_backend>
int NvmeZns<nvme_zns_backend>::append(
    uint64_t zone, uint64_t& sector, uint64_t offset, void* buffer, uint64_t size)
{
    return this->backend->append(zone, sector, offset, buffer, size);
}

template<class nvme_zns_backend>
int NvmeZns<nvme_zns_backend>::reset(uint64_t zone) {
    return this->backend->reset(zone);
}
